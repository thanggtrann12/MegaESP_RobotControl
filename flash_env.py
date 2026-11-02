#!/usr/bin/env python3
"""Tkinter GUI for PlatformIO - Modern Dashboard Style.

Run:  python flash_env.py [project_dir]
"""

import configparser
import http.server
import json
import queue
import shutil
import socket
import socketserver
import subprocess
import sys
import threading
import time
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

IS_WIN = sys.platform.startswith("win")
AUTO_PORT = "(from platformio.ini)"

FONT = ("Segoe UI" if IS_WIN else "Helvetica", 10)
MONO = "Consolas" if IS_WIN else "monospace"

# Bảng màu chuẩn theo mẫu thiết kế
THEME = {
    "bg": "#0e131f",
    "card": "#151b2b",
    "card_hover": "#1d263b",
    "log_bg": "#0b0f19",
    "fg": "#f0f6fc",
    "muted": "#8b949e",
    "border": "#21283b",
    "btn_bg": "#1f2638",
    "btn_border": "#303b54",
    "btn_hover": "#28324a",
    "accent_bg": "#2563eb",
    "accent_hover": "#3b82f6",
    "danger_bg": "#dc2626",
    "danger_hover": "#ef4444",
    "ok": "#22c55e",
    "warn": "#eab308",
}

PLATFORM_COLORS = {
    "espressif32": "#3b82f6",
    "espressif8266": "#a855f7",
    "atmelavr": "#eab308"
}


def find_pio():
    for name in ("pio", "platformio"):
        path = shutil.which(name)
        if path:
            return [path]
    try:
        import platformio  # noqa: F401
        return [sys.executable, "-m", "platformio"]
    except ImportError:
        return None


def load_ini(project):
    ini = Path(project) / "platformio.ini"
    if not ini.is_file():
        return None
    cfg = configparser.ConfigParser(interpolation=None, strict=False, inline_comment_prefixes=(";",))
    cfg.read(ini, encoding="utf-8")
    return cfg


def read_envs(project):
    cfg = load_ini(project)
    if cfg is None:
        return []
    return [s.split(":", 1)[1] for s in cfg.sections() if s.startswith("env:")]


def env_option(project, env, key, _seen=None):
    cfg = load_ini(project)
    if cfg is None:
        return None
    seen = _seen or set()
    section = f"env:{env}"
    if section in seen:
        return None
    seen.add(section)
    if cfg.has_option(section, key):
        return cfg.get(section, key).strip() or None
    if cfg.has_option(section, "extends"):
        for parent in cfg.get(section, "extends").replace(",", "\n").split():
            parent = parent.strip()
            if not parent:
                continue
            name = parent.split(":", 1)[1] if parent.startswith("env:") else parent
            if cfg.has_section(parent) and not parent.startswith("env:"):
                if cfg.has_option(parent, key):
                    return cfg.get(parent, key).strip() or None
            else:
                value = env_option(project, name, key, seen)
                if value:
                    return value
    if cfg.has_option("env", key):
        return cfg.get("env", key).strip() or None
    return None


def ini_ports(project, env):
    up = env_option(project, env, "upload_port")
    mon = env_option(project, env, "monitor_port") or up
    return up, mon


NET_LOG_PORT = 23  # TCP log server of the OTA bridge firmware
OTA_HTTP_PORT = 8000
OTA_FW_WARNING = (
    "KHÔNG TẮT NGUỒN hoặc rút dây trong khi cập nhật firmware hub!\n\n"
    "- Hub tải file .bin mới nhất từ OTA server, kiểm tra sha256, ghi rồi tự khởi động lại.\n"
    "- Không tắt nguồn hub/router, không rút cáp USB, không tắt OTA server cho đến khi hub khởi động lại.\n"
    "- Robot sẽ dừng và mất liên kết ESP-NOW trong lúc cập nhật.\n"
    "- Nếu lỗi (đứt mạng, sai sha256) hub giữ nguyên firmware cũ.\n\n"
    "Bấm OK để bắt đầu."
)
OTA_WARNING = (
    "KHÔNG TẮT NGUỒN hoặc rút dây trong khi cập nhật màn hình!\n\n"
    "- Không tắt/cấp lại nguồn hub, màn hình TJC, router; không rút cáp USB hoặc dây UART.\n"
    "- Không tắt server OTA hoặc đóng tool cho đến khi hub tự khởi động lại.\n"
    "- Quá trình mất khoảng 10 phút; robot sẽ dừng và mất liên kết ESP-NOW.\n"
    "- Nếu bị ngắt giữa chừng, màn hình có thể hỏng giao diện và phải nạp lại bằng thẻ SD / USB-TTL.\n\n"
    "Bấm OK để bắt đầu."
)  # local server the hub downloads the screen firmware from


def local_ip():
    """LAN address of this PC (the one the default route uses); the hub downloads from it."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("10.255.255.255", 1))  # no packet is sent, it only selects the interface
        return s.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        s.close()


def make_file_handler(file_path, log):
    """HTTP handler that serves one file, at /<its name>, and reports every request."""
    name = Path(file_path).name

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path.lstrip("/").split("?")[0] != name:
                self.send_error(404)
                log(f"[server] {self.client_address[0]} GET {self.path} -> 404\n", "warn")
                return
            data = Path(file_path).read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            log(f"[server] {self.client_address[0]} GET /{name} ({len(data)} bytes)\n", "cmd")
            try:
                self.wfile.write(data)
                log(f"[server] sent {len(data)} bytes to {self.client_address[0]}\n", "ok")
            except OSError as exc:
                log(f"[server] transfer aborted: {exc}\n", "err")

        def log_message(self, *_args):
            pass

    return Handler


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

# Runs in its own console: streams the bridge's TCP log until Ctrl+C / window close.
TCP_LOG_SCRIPT = (
    "import os,socket,sys,time\n"
    "os.system('')\n"
    "h,p=sys.argv[1],int(sys.argv[2])\n"
    "print('TCP log ->',h,p,'(Ctrl+C to quit)',flush=True)\n"
    "while True:\n"
    "    try:\n"
    "        s=socket.create_connection((h,p),timeout=5)\n"
    "        s.settimeout(None)\n"
    "        print('--- connected ---',flush=True)\n"
    "        while True:\n"
    "            d=s.recv(4096)\n"
    "            if not d: break\n"
    "            sys.stdout.write(d.decode(errors='replace')); sys.stdout.flush()\n"
    "        print('--- closed, reconnecting ---',flush=True)\n"
    "    except KeyboardInterrupt: break\n"
    "    except OSError as e: print('--- error:',e,'- retry in 2s ---',flush=True)\n"
    "    time.sleep(2)\n"
)

class Tooltip:
    """Small dark tooltip shown after hovering a widget for a moment."""

    def __init__(self, widget, text, delay=400):
        self.widget, self.text, self.delay = widget, text, delay
        self.tip = None
        self.job = None
        widget.bind("<Enter>", self._schedule, add="+")
        widget.bind("<Leave>", self._hide, add="+")
        widget.bind("<ButtonPress>", self._hide, add="+")

    def _schedule(self, _event=None):
        self._cancel()
        self.job = self.widget.after(self.delay, self._show)

    def _cancel(self):
        if self.job:
            self.widget.after_cancel(self.job)
            self.job = None

    def _show(self):
        if self.tip or not self.text:
            return
        x = self.widget.winfo_rootx() + 8
        y = self.widget.winfo_rooty() + self.widget.winfo_height() + 6
        self.tip = tk.Toplevel(self.widget)
        self.tip.wm_overrideredirect(True)
        self.tip.wm_geometry(f"+{x}+{y}")
        tk.Label(self.tip, text=self.text, bg=THEME["btn_hover"], fg=THEME["fg"], font=(FONT[0], 9),
                 padx=8, pady=4, highlightthickness=1, highlightbackground=THEME["border"]).pack()

    def _hide(self, _event=None):
        self._cancel()
        if self.tip:
            self.tip.destroy()
            self.tip = None


class ModernButton(tk.Button):
    """Nút bấm nổi, bo góc và hiệu ứng tương tác trực quan."""
    def __init__(self, parent, text, command=None, variant="normal", tip=None, **kwargs):
        bg = THEME["btn_bg"]
        border = THEME["btn_border"]
        hover = THEME["btn_hover"]
        fg = THEME["fg"]

        if variant == "accent":
            bg = THEME["accent_bg"]
            border = "#3b82f6"
            hover = THEME["accent_hover"]
            fg = "#ffffff"
        elif variant == "danger":
            bg = THEME["danger_bg"]
            border = "#ef4444"
            hover = THEME["danger_hover"]
            fg = "#ffffff"

        self.normal_bg = bg
        self.normal_fg = fg
        self.hover_bg = hover
        self.normal_border = border

        super().__init__(
            parent,
            text=text,
            command=command,
            bg=bg,
            fg=fg,
            activebackground=hover,
            activeforeground=fg,
            font=(FONT[0], 11),
            relief="raised",
            bd=1,
            highlightthickness=1,
            highlightbackground=border,
            highlightcolor=border,
            padx=16,
            pady=7,
            cursor="hand2",
            **kwargs
        )
        if tip:
            Tooltip(self, tip)
        self.bind("<Enter>", self._on_enter, add="+")
        self.bind("<Leave>", self._on_leave, add="+")

    def _on_enter(self, e):
        if self["state"] != "disabled":
            self.configure(bg=self.hover_bg)

    def _on_leave(self, e):
        if self["state"] != "disabled":
            self.configure(bg=self.normal_bg)

    def set_state(self, state):
        if state == "disabled":
            self.configure(state="disabled", bg=THEME["card"], fg=THEME["muted"], relief="flat", cursor="arrow")
        else:
            self.configure(state="normal", bg=self.normal_bg, fg=self.normal_fg, relief="raised", cursor="hand2")


class WrapFrame(ttk.Frame):
    """Flex-wrap row: children flow left to right and wrap onto new lines when the width runs out."""

    def __init__(self, parent, gap=6, **kwargs):
        super().__init__(parent, **kwargs)
        self.gap = gap
        self.bind("<Configure>", self._reflow)
        self.after_idle(self._reflow)

    def _reflow(self, _event=None):
        width = self.winfo_width()
        x = y = row_h = 0
        for child in self.winfo_children():
            w, h = child.winfo_reqwidth(), child.winfo_reqheight()
            if x and x + w > width:
                x, y, row_h = 0, y + row_h + self.gap, 0
            child.place(x=x, y=y)
            x += w + self.gap
            row_h = max(row_h, h)
        if int(self.cget("height") or 0) != y + row_h:
            self.configure(height=y + row_h)


class CardGrid(tk.Frame):
    """Responsive grid: as many equal-width columns as fit `min_col` pixels, reflowing on resize."""

    def __init__(self, parent, min_col=430, gap=8, **kwargs):
        super().__init__(parent, **kwargs)
        self.min_col, self.gap = min_col, gap
        self.bind("<Configure>", self._reflow)

    def _reflow(self, _event=None):
        width = self.winfo_width()
        cards = self.winfo_children()
        if not cards or width < 2:
            return
        cols = max(1, min(len(cards), (width + self.gap) // (self.min_col + self.gap)))
        col_w = (width - self.gap * (cols - 1)) // cols
        y = 0
        for start in range(0, len(cards), cols):
            row = cards[start:start + cols]
            row_h = max(c.winfo_reqheight() for c in row)
            for i, card in enumerate(row):
                card.place(x=i * (col_w + self.gap), y=y, width=col_w, height=row_h)
            y += row_h + self.gap
        total = max(y - self.gap, 1)
        if int(self.cget("height") or 0) != total:
            self.configure(height=total)

    def relayout(self):
        self.after_idle(self._reflow)


def autowrap(label, container, margin=40):
    """Make a Label's wraplength follow its container width."""
    container.bind("<Configure>", lambda e: label.configure(wraplength=max(120, e.width - margin)), add="+")


class App(tk.Tk):
    def __init__(self, project):
        super().__init__()
        self.title("PlatformIO Flash Hub")
        self.geometry("1000x860")
        self.pio = find_pio()
        self.running = False
        self.procs = []  # pio processes of the current run (several when flashing in parallel)
        self.selected = {}  # env name -> BooleanVar of its checkbox
        self.lines = queue.Queue()
        self.buttons = []
        self.log_buttons = []
        self.log_write_failed = False
        self.on_done = None
        self.last_code = 0
        self.monitors = {}  # port (or host:23) -> monitor console process
        self.ota_file = tk.StringVar()
        self.ota_url = tk.StringVar(value="(chưa chạy server)")
        self.ota_dry = tk.BooleanVar(value=False)
        self.ota_server = None

        self.project = tk.StringVar(value=str(Path(project).resolve()))
        self.port = tk.StringVar()
        self.verbose = tk.BooleanVar(value=False)
        self.ini_stamp = None

        self._build_ui()
        self.reload_envs()
        self.refresh_ports()
        self.after(50, self._drain)
        self.after(1000, self._watch_ini)

        if self.pio is None:
            self.after(200, self.install_pio)

    def install_pio(self):
        self.clear_log()
        self.write("PlatformIO not found, installing with pip...\n", "cmd")
        steps = [[sys.executable, "-m", "pip", "install", "platformio"]]
        self.on_done = self._after_install
        self._set_busy(True, "installing platformio")
        threading.Thread(target=self._run_jobs, args=([(None, "", steps)], self.project.get()),
                         daemon=True).start()

    def _after_install(self):
        self.pio = find_pio()
        if self.pio is None:
            messagebox.showerror("PlatformIO", "Install failed.\nRun manually: pip install platformio")
            return
        self.write("PlatformIO installed.\n", "ok")
        self.refresh_ports()

    def _apply_theme(self):
        self.configure(bg=THEME["bg"])
        s = ttk.Style(self)
        s.theme_use("clam")
        s.configure(".", background=THEME["bg"], foreground=THEME["fg"], font=FONT, borderwidth=0)
        s.configure("TFrame", background=THEME["bg"])
        s.configure("Card.TFrame", background=THEME["card"])
        s.configure("Title.TLabel", font=(FONT[0], 15, "bold"), foreground=THEME["fg"])
        s.configure("Env.TLabel", background=THEME["card"], font=(FONT[0], 11, "bold"), foreground=THEME["fg"])
        s.configure("Sub.TLabel", background=THEME["card"], font=(FONT[0], 9), foreground=THEME["muted"])
        s.configure("Muted.TLabel", background=THEME["card"], foreground=THEME["muted"])
        s.configure("Status.TLabel", foreground=THEME["fg"], font=(FONT[0], 9, "bold"))
        s.configure("TCheckbutton", background=THEME["bg"], foreground=THEME["fg"])
        s.map("TCheckbutton", background=[("active", THEME["bg"])])
        s.configure("TCombobox", fieldbackground=THEME["card"], background=THEME["card"],
                    foreground=THEME["fg"], arrowcolor=THEME["fg"], padding=6)
        s.map("TCombobox", fieldbackground=[("readonly", THEME["card"])])
        self.option_add("*TCombobox*Listbox.background", THEME["card"])
        self.option_add("*TCombobox*Listbox.foreground", THEME["fg"])
        s.configure("Vertical.TScrollbar", background=THEME["btn_bg"], troughcolor=THEME["bg"], arrowcolor=THEME["fg"])
        s.configure("Busy.Horizontal.TProgressbar", background=THEME["accent_bg"], troughcolor=THEME["card"], thickness=3)

    def _build_ui(self):
        self._apply_theme()
        self.minsize(760, 520)

        # Controls reflow to the window width (wrapping rows, multi-column cards); the log takes the rest.
        top = ttk.Frame(self)
        top.pack(fill="x")
        bottom = ttk.Frame(self)
        bottom.pack(fill="both", expand=True)
        # Keep room for the log: the window can't shrink below controls + ~180 px of log.
        top.bind("<Configure>", lambda e: self.minsize(760, e.height + 200))

        # 1. Header Section
        header = ttk.Frame(top, padding=(20, 16, 20, 8))
        header.pack(fill="x")

        ttk.Label(header, text="⚡ PlatformIO Flash Hub", style="Title.TLabel").pack(side="left")

        # Trạng thái kiểu Badge góc phải trên
        self.status_badge = tk.Frame(header, bg=THEME["card"], highlightbackground=THEME["border"], highlightthickness=1)
        self.status_badge.pack(side="right")
        status_inner = ttk.Frame(self.status_badge, padding=(10, 4), style="Card.TFrame")
        status_inner.pack()
        self.dot = tk.Canvas(status_inner, width=8, height=8, bg=THEME["card"], highlightthickness=0)
        self.dot.pack(side="left", padx=(0, 6))
        self.dot_item = self.dot.create_oval(0, 0, 8, 8, fill=THEME["ok"], outline="")
        self.status = ttk.Label(status_inner, text="Ready", style="Status.TLabel")
        self.status.pack(side="left")

        self.progress = ttk.Progressbar(top, mode="indeterminate", style="Busy.Horizontal.TProgressbar")

        # 2. Project Card Section (Giống mẫu)
        proj_outer = tk.Frame(top, bg=THEME["border"])
        proj_outer.pack(fill="x", padx=20, pady=6)
        proj_card = tk.Frame(proj_outer, bg=THEME["card"])
        proj_card.pack(fill="x", padx=1, pady=1)

        proj_inner = ttk.Frame(proj_card, style="Card.TFrame", padding=14)
        proj_inner.pack(fill="x")

        proj_top = ttk.Frame(proj_inner, style="Card.TFrame")
        proj_top.pack(fill="x", anchor="w")
        ttk.Label(proj_top, text="PROJECT", style="Sub.TLabel", font=(FONT[0], 8, "bold")).pack(anchor="w")

        proj_bottom = ttk.Frame(proj_inner, style="Card.TFrame")
        proj_bottom.pack(fill="x", pady=(4, 0))

        # Tên project hiển thị dạng text/nhãn nổi bật kèm ô chọn folder thu gọn
        self.proj_lbl = ttk.Label(proj_bottom, textvariable=self.project, style="Env.TLabel", font=(FONT[0], 12, "bold"))
        self.proj_lbl.pack(side="left", anchor="w")

        btn_browse = ModernButton(proj_bottom, text="📁 Chọn thư mục", command=self.browse,
                                  tip="Chọn thư mục project chứa platformio.ini")
        btn_browse.pack(side="right")

        port_row = ttk.Frame(proj_inner, style="Card.TFrame")
        port_row.pack(fill="x", pady=(10, 0))
        ttk.Label(port_row, text="PORT", style="Sub.TLabel", font=(FONT[0], 8, "bold")).pack(side="left", padx=(0, 10))
        self.port_box = ttk.Combobox(port_row, textvariable=self.port, state="readonly")
        self.port_box.pack(side="left", fill="x", expand=True)
        btn_refresh = ModernButton(port_row, text="↻ Refresh", command=self.refresh_ports,
                                   tip="Quét lại danh sách cổng COM")
        btn_refresh.pack(side="left", padx=(8, 0))
        ttk.Checkbutton(port_row, text="Verbose", variable=self.verbose).pack(side="left", padx=(12, 0))

        # 2b. HMI screen OTA test server
        ota_outer = tk.Frame(top, bg=THEME["border"])
        ota_outer.pack(fill="x", padx=20, pady=6)
        ota_card = ttk.Frame(ota_outer, style="Card.TFrame", padding=14)
        ota_card.pack(fill="x", padx=1, pady=1)
        ttk.Label(ota_card, text="HMI SCREEN OTA (TEST SERVER)", style="Sub.TLabel",
                  font=(FONT[0], 8, "bold")).pack(anchor="w")

        lbl_file = ttk.Label(ota_card, textvariable=self.ota_file, style="Env.TLabel", justify="left")
        lbl_file.pack(fill="x", pady=(6, 0))
        autowrap(lbl_file, ota_card)
        lbl_url = ttk.Label(ota_card, textvariable=self.ota_url, style="Muted.TLabel", justify="left")
        lbl_url.pack(fill="x", pady=(4, 0))
        autowrap(lbl_url, ota_card)

        server_row = WrapFrame(ota_card, style="Card.TFrame", height=30)
        server_row.pack(fill="x", pady=(8, 0))
        ModernButton(server_row, text="📄 Chọn file .tft", command=self.pick_ota_file,
                     tip="Chọn file firmware màn hình (.tft) để hub tải về")
        self.ota_start_btn = ModernButton(server_row, text="▶ Start server", command=self.start_ota_server,
                                          tip="Chạy server HTTP local phục vụ file đã chọn")
        self.ota_stop_btn = ModernButton(server_row, text="■ Stop server", variant="danger",
                                         command=self.stop_ota_server, tip="Tắt server HTTP local")
        self.ota_stop_btn.set_state("disabled")

        send_row = WrapFrame(ota_card, style="Card.TFrame", height=30)
        send_row.pack(fill="x", pady=(8, 0))
        ttk.Checkbutton(send_row, text="Dry run (hub chỉ tải về, không đẩy xuống màn hình)",
                        variable=self.ota_dry)
        btn_send = ModernButton(send_row, text="📡 Gửi lệnh TFTOTA tới hub", variant="accent",
                                command=self.trigger_ota,
                                tip="Gửi TFTOTA qua cổng PORT ở trên: hub tải file từ server này")
        self.buttons.append(btn_send)
        btn_fw = ModernButton(send_row, text="⬆ Cập nhật firmware hub", variant="accent",
                              command=self.trigger_hub_fw,
                              tip="Gửi FWOTA: hub tải .bin mới nhất từ server (hub tự tải bản mới nhất, Dry run dùng chung), "
                                  "kiểm tra sha256, ghi và khởi động lại")
        self.buttons.append(btn_fw)

        # 3. Targets Header Bar & Danh sách môi trường
        targets_bar = WrapFrame(top, height=36)
        targets_bar.pack(fill="x", padx=20, pady=(8, 4))

        self.targets_count_lbl = ttk.Label(targets_bar, text="0 firmware targets", font=(FONT[0], 10, "bold"))
        for text, variant, tip, cmd in (
            ("☑ Chọn tất cả", "normal", "Tick hoặc bỏ tick tất cả target", self.toggle_all),
            ("⚙ Build đã chọn", "normal", "Build các target đã tick cùng lúc",
             lambda: self.run_selected("build")),
            ("⬆ Upload đã chọn", "accent", "Flash các target đã tick cùng lúc (cổng khác nhau chạy song song)",
             lambda: self.run_selected("upload")),
            ("Build All", "accent", "Build tất cả môi trường", lambda: self.run_pio(None, "build")),
            ("Clean", "normal", "Xóa kết quả build của tất cả môi trường", lambda: self.run_pio(None, "clean")),
        ):
            self.buttons.append(ModernButton(targets_bar, text=text, variant=variant, command=cmd, tip=tip))
        self.top_buttons = list(self.buttons)

        # Vùng chứa các card envs động, tự chia cột theo bề rộng cửa sổ
        env_host = ttk.Frame(top, padding=(20, 2))
        env_host.pack(fill="x")
        self.env_frame = CardGrid(env_host, bg=THEME["bg"], height=1)
        self.env_frame.pack(fill="x")

        # 4. Activity Log Section (Đúng chuẩn mẫu)
        log_header = ttk.Frame(bottom, padding=(20, 10, 20, 4))
        log_header.pack(fill="x")
        ttk.Label(log_header, text="Activity Log", font=(FONT[0], 11, "bold")).pack(side="left")

        self.stop_btn = ModernButton(log_header, text="■ Stop", variant="danger", command=self.stop,
                                     tip="Dừng lệnh đang chạy")
        self.stop_btn.pack(side="right")
        self.stop_btn.set_state("disabled")
        export_btn = ModernButton(log_header, text="⤓ Export", command=self.export_log,
                                  tip="Lưu log ra file .pro")
        export_btn.pack(side="right", padx=6)
        clear_btn = ModernButton(log_header, text="✕ Clear", command=self.clear_log,
                                 tip="Xóa log hiện tại")
        clear_btn.pack(side="right")
        self.log_buttons.extend((clear_btn, export_btn))

        log_container = tk.Frame(bottom, bg=THEME["border"])
        log_container.pack(fill="both", padx=20, pady=(0, 16), expand=True)
        log_inner = tk.Frame(log_container, bg=THEME["log_bg"])
        log_inner.pack(fill="both", padx=1, pady=1, expand=True)

        self.log = tk.Text(log_inner, wrap="none", bg=THEME["log_bg"], fg=THEME["fg"], insertbackground=THEME["fg"],
                           relief="flat", padx=12, pady=10, highlightthickness=0,
                           font=(MONO, 11))
        sb = ttk.Scrollbar(log_inner, command=self.log.yview)
        self.log.configure(yscrollcommand=sb.set)
        self.log.pack(side="left", fill="both", expand=True)
        sb.pack(side="right", fill="y")

        self.log.tag_config("err", foreground=THEME["danger_bg"])
        self.log.tag_config("ok", foreground=THEME["ok"])
        self.log.tag_config("warn", foreground=THEME["warn"])
        self.log.tag_config("cmd", foreground=THEME["accent_bg"])

        # Dòng thông điệp mặc định khi chưa có hoạt động
        self.write("Chưa có hoạt động. Output sẽ xuất hiện tại đây.\n", "warn")

    def browse(self):
        path = filedialog.askdirectory(initialdir=self.project.get())
        if path:
            self.project.set(path)
            self.reload_envs()

    def _ini_stamp(self):
        ini = Path(self.project.get()) / "platformio.ini"
        try:
            return ini.stat().st_mtime_ns
        except OSError:
            return None

    def _watch_ini(self):
        if not self.running and self._ini_stamp() != self.ini_stamp:
            self.reload_envs()
        self.after(1000, self._watch_ini)

    def reload_envs(self):
        self.ini_stamp = self._ini_stamp()
        for child in self.env_frame.winfo_children():
            child.destroy()
        self.buttons[:] = self.top_buttons

        envs = read_envs(self.project.get())
        self.selected = {e: self.selected.get(e) or tk.BooleanVar(value=False) for e in envs}
        self.targets_count_lbl.configure(text=f"{len(envs)} firmware targets")

        if not envs:
            lbl = ttk.Label(self.env_frame, text="⚠ Không tìm thấy phần [env:...] trong platformio.ini", foreground=THEME["warn"])
            self.env_frame.relayout()
            return

        for env in envs:
            project = self.project.get()
            up, mon = ini_ports(project, env)
            platform = env_option(project, env, "platform")
            board = env_option(project, env, "board")

            # Chuẩn bị chuỗi thông tin phụ (Ví dụ: ESP32-S3 · COM13)
            info_parts = [p for p in (platform, board, up or mon, "Chưa chọn" if not (up or mon) else "") if p]
            sub_text = " · ".join(info_parts)

            self._env_card(env, sub_text, PLATFORM_COLORS.get(platform, THEME["accent_bg"]), [
                ("⚙", "normal", "Build môi trường này", lambda e=env: self.run_pio(e, "build")),
                ("⬆", "accent", "Upload firmware (flash)", lambda e=env: self.run_pio(e, "upload")),
                ("▶", "normal", "Mở serial monitor", lambda e=env: self.run_pio(e, "monitor")),
                ("✕", "normal", "Clean môi trường này", lambda e=env: self.run_pio(e, "clean")),
            ])

    def _env_card(self, title, sub_text, accent_color, actions):
        outer = tk.Frame(self.env_frame, bg=THEME["border"])
        self.env_frame.relayout()

        inner_card = tk.Frame(outer, bg=THEME["card"])
        inner_card.pack(fill="x", padx=1, pady=1)

        # Vạch màu đánh dấu bên trái
        tk.Frame(inner_card, bg=accent_color, width=4).pack(side="left", fill="y")

        content = ttk.Frame(inner_card, style="Card.TFrame", padding=(12, 10))
        content.pack(side="left", fill="both", expand=True)

        chk = tk.Checkbutton(content, variable=self.selected[title], bg=THEME["card"], fg=THEME["fg"],
                             activebackground=THEME["card"], selectcolor=THEME["bg"],
                             highlightthickness=0, bd=0, cursor="hand2")
        chk.pack(side="left", padx=(0, 10))
        Tooltip(chk, "Tick để flash cùng lúc với các target khác")

        # Thông tin tên môi trường và thông số bên trái
        left_info = ttk.Frame(content, style="Card.TFrame")
        left_info.pack(side="left", fill="y", anchor="w")

        title_lbl = ttk.Label(left_info, text=title, style="Env.TLabel")
        title_lbl.pack(side="top", anchor="w")

        sub_lbl = ttk.Label(left_info, text=sub_text, style="Sub.TLabel")
        sub_lbl.pack(side="top", anchor="w", pady=(2, 0))

        # Cụm nút hành động bên phải card
        right_actions = ttk.Frame(content, style="Card.TFrame")
        right_actions.pack(side="right", fill="y")

        for text, variant, tip, cmd in actions:
            btn = ModernButton(right_actions, text=text, variant=variant, command=cmd, tip=tip)
            btn.pack(side="left", padx=3)
            self.buttons.append(btn)

    def refresh_ports(self):
        if not self.pio:
            return

        def scan():
            try:
                out = subprocess.run(self.pio + ["device", "list", "--json-output"],
                                     capture_output=True, text=True, timeout=20).stdout
                ports = [d["port"] for d in json.loads(out)]
            except Exception:
                ports = []
            self.after(0, lambda: self._apply_ports(ports))

        threading.Thread(target=scan, daemon=True).start()

    def _apply_ports(self, ports):
        self.port_box["values"] = [AUTO_PORT] + ports
        if not self.port.get() or self.port.get() not in self.port_box["values"]:
            self.port.set(AUTO_PORT)

    def _log_file_path(self):
        return Path(self.project.get()) / "flash_log.pro"

    def clear_log(self):
        self.log.delete("1.0", "end")
        self.log_write_failed = False
        try:
            self._log_file_path().write_text("", encoding="utf-8")
        except OSError as exc:
            self.log_write_failed = True
            messagebox.showerror("Lỗi", f"Không thể xóa tệp log:\n{exc}")

    def export_log(self):
        filename = filedialog.asksaveasfilename(
            title="Xuất tệp nhật ký",
            initialdir=self.project.get(),
            initialfile="flash_log.pro",
            defaultextension=".pro",
            filetypes=(("PlatformIO log (*.pro)", "*.pro"),),
        )
        if not filename:
            return
        try:
            Path(filename).write_text(self.log.get("1.0", "end-1c"), encoding="utf-8")
        except OSError as exc:
            messagebox.showerror("Lỗi", f"Không thể lưu tệp:\n{exc}")
            return
        messagebox.showinfo("Thành công", f"Đã xuất log tới:\n{filename}")

    def write(self, text, tag=None):
        self.log.insert("end", text, tag)
        self.log.see("end")
        if not self.log_write_failed:
            try:
                with self._log_file_path().open("a", encoding="utf-8") as log_file:
                    log_file.write(text)
            except OSError as exc:
                self.log_write_failed = True
                messagebox.showerror("Lỗi", f"Không thể ghi log:\n{exc}")

    def _pio_steps(self, project, env, action, override=None):
        base = self.pio + ["run", "-d", project]
        if env:
            base += ["-e", env]
        if self.verbose.get():
            base.append("-v")
        if action == "build":
            return [base]
        if action == "clean":
            return [base + ["-t", "clean"]]
        up = base + ["-t", "upload"]
        if override:
            up += ["--upload-port", override]
        return [up]

    def toggle_all(self):
        value = not all(v.get() for v in self.selected.values())
        for var in self.selected.values():
            var.set(value)

    def run_selected(self, action):
        if self.running:
            messagebox.showinfo("Đang bận", "Tiến trình khác đang chạy.")
            return
        envs = [e for e, var in self.selected.items() if var.get()]
        if not envs:
            messagebox.showinfo("Chưa chọn target", "Tick vào ít nhất một môi trường.")
            return
        if not self.pio:
            return
        self.clear_log()
        project = self.project.get()

        # Envs sharing one upload port must run one after another; different ports run in parallel.
        jobs = []
        for env in envs:
            lane = env
            if action == "upload":
                port = ini_ports(project, env)[0]
                self.close_monitors(port)
                lane = port or env
            jobs.append((lane, env, self._pio_steps(project, env, action)))

        self._set_busy(True, f"Running {action} x{len(envs)}...")
        threading.Thread(target=self._run_jobs, args=(jobs, project), daemon=True).start()

    def run_pio(self, env, action):
        if self.running:
            messagebox.showinfo("Đang bận", "Tiến trình khác đang chạy.")
            return
        if not self.pio:
            return
        self.clear_log()
        project = self.project.get()
        port = self.port.get()
        override = None if port in ("", AUTO_PORT, "(auto)") else port

        steps = []
        if action in ("build", "clean"):
            steps = self._pio_steps(project, env, action)
        elif action == "upload":
            up_port = override or ini_ports(project, env)[0]
            self.close_monitors(up_port)
            steps = self._pio_steps(project, env, action, override)
        elif action == "monitor":
            if env_option(project, env, "upload_protocol") == "espota":
                # OTA board has no USB serial: read its TCP log server in a separate console.
                host = ini_ports(project, env)[0]
                if not host:
                    self.write("No upload_port (IP) set for this environment.\n", "err")
                    return
                key = f"{host}:{NET_LOG_PORT}"
                self.close_monitors(key, serial_only=False)
                self.write(f"Opening TCP log terminal for {key}\n", "cmd")
                self.monitors[key] = subprocess.Popen(
                    [sys.executable, "-c", TCP_LOG_SCRIPT, host, str(NET_LOG_PORT)],
                    creationflags=subprocess.CREATE_NEW_CONSOLE if IS_WIN else 0,
                )
                return
            mon = self.pio + ["device", "monitor", "-d", project, "-e", env]
            mon_port = override or ini_ports(project, env)[1]
            if mon_port:
                mon += ["--port", mon_port]
            if IS_WIN:
                # Only a monitor already holding the same port is replaced; others stay open.
                key = mon_port or f"{env} (auto)"
                self.close_monitors(key, serial_only=False)
                self.write("$ " + " ".join(mon) + "\n", "cmd")
                self.monitors[key] = subprocess.Popen(
                    mon, creationflags=subprocess.CREATE_NEW_CONSOLE)
                return
            steps = [mon]

        self._set_busy(True, f"Running {action}...")
        threading.Thread(target=self._run_jobs, args=([(None, "", steps)], project), daemon=True).start()

    def close_monitors(self, port, serial_only=True):
        """Close monitor windows that hold `port` (all serial ones if the port is unknown)."""
        for key, proc in list(self.monitors.items()):
            if proc.poll() is not None:
                del self.monitors[key]
                continue
            is_net = key.endswith(f":{NET_LOG_PORT}")
            if serial_only and is_net:
                continue  # a TCP log never blocks an upload
            if port and key.lower() != port.lower():
                continue
            del self.monitors[key]
            self.write(f"Closing the monitor on {key} to free the port...\n", "warn")
            # /T also kills the python child that actually holds the port
            subprocess.run(["taskkill", "/PID", str(proc.pid), "/T", "/F"], capture_output=True)
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                pass
            time.sleep(0.5)  # give Windows a moment to release the COM handle

    def _run_job(self, label, steps, project, results):
        prefix = f"[{label}] " if label else ""
        code = 0
        for cmd in steps:
            self.lines.put((f"{prefix}$ " + " ".join(cmd) + "\n", "cmd"))
            proc = None
            try:
                proc = subprocess.Popen(cmd, cwd=project, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, text=True, bufsize=1,
                                        errors="replace")
                self.procs.append(proc)
                for line in proc.stdout:
                    low = line.lower()
                    if "error" in low or "failed" in low:
                        tag = "err"
                    elif "warning" in low:
                        tag = "warn"
                    elif "success" in low:
                        tag = "ok"
                    else:
                        tag = None
                    self.lines.put((prefix + line, tag))
                code = proc.wait()
            except Exception as exc:
                self.lines.put((f"{prefix}{exc}\n", "err"))
                code = 1
            finally:
                if proc in self.procs:
                    self.procs.remove(proc)
            if code != 0:
                break
        results[label] = code

    def _run_jobs(self, jobs, project):
        """jobs: (lane, label, steps). Each lane runs in its own thread; jobs in a lane run in order."""
        lanes = {}
        for lane, label, steps in jobs:
            lanes.setdefault(lane, []).append((label, steps))
        results = {}

        def run_lane(items):
            for label, steps in items:
                self._run_job(label, steps, project, results)

        threads = [threading.Thread(target=run_lane, args=(items,), daemon=True)
                   for items in lanes.values()]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        if len(results) > 1:
            self.lines.put(("\n--- Summary ---\n", "cmd"))
            for label, c in results.items():
                self.lines.put((f"{label}: {'OK' if c == 0 else f'FAILED ({c})'}\n",
                                "ok" if c == 0 else "err"))
        code = next((c for c in results.values() if c != 0), 0)
        self.last_code = code
        self.lines.put((f"\n=== {'DONE' if code == 0 else f'FAILED ({code})'} ===\n",
                        "ok" if code == 0 else "err"))
        self.lines.put(None)

    def _drain(self):
        try:
            while True:
                item = self.lines.get_nowait()
                if item is None:
                    if self.last_code == 0:
                        self._set_busy(False, "Ready", THEME["ok"])
                    else:
                        self._set_busy(False, "Error", THEME["danger_bg"])
                    done, self.on_done = self.on_done, None
                    if done:
                        done()
                else:
                    self.write(*item)
        except queue.Empty:
            pass
        self.after(50, self._drain)

    # ------------------------------------------------------------------ HMI screen OTA test
    def pick_ota_file(self):
        path = filedialog.askopenfilename(
            title="Chọn file firmware màn hình",
            initialdir=self.project.get(),
            filetypes=(("TJC firmware", "*.tft"), ("All files", "*.*")),
        )
        if not path:
            return
        self.ota_file.set(path)
        if self.ota_server:  # keep serving, but the new file replaces the old one
            self.stop_ota_server()
            self.start_ota_server()

    def _ota_file_url(self):
        return f"http://{local_ip()}:{OTA_HTTP_PORT}/{Path(self.ota_file.get()).name}"

    def start_ota_server(self):
        if self.ota_server:
            return
        path = self.ota_file.get()
        if not path or not Path(path).is_file():
            messagebox.showwarning("OTA", "Hãy chọn file .tft trước.")
            return
        handler = make_file_handler(path, lambda text, tag=None: self.lines.put((text, tag)))
        try:
            self.ota_server = ThreadedServer(("0.0.0.0", OTA_HTTP_PORT), handler)
        except OSError as exc:
            messagebox.showerror("OTA", f"Không mở được cổng {OTA_HTTP_PORT}:\n{exc}")
            return
        threading.Thread(target=self.ota_server.serve_forever, daemon=True).start()
        url = self._ota_file_url()
        self.ota_url.set(url)
        self.ota_start_btn.set_state("disabled")
        self.ota_stop_btn.set_state("normal")
        self.write(f"OTA server running: {url}\n", "ok")
        self.write("Nếu hub không tải được, cho phép Python qua Windows Firewall (mạng Private).\n", "warn")

    def stop_ota_server(self):
        if self.ota_server:
            self.ota_server.shutdown()
            self.ota_server.server_close()
            self.ota_server = None
            self.write("OTA server stopped.\n", "warn")
        self.ota_url.set("(chưa chạy server)")
        self.ota_start_btn.set_state("normal")
        self.ota_stop_btn.set_state("disabled")

    def _hub_port(self):
        port = self.port.get()
        if port in ("", AUTO_PORT, "(auto)"):
            port = ini_ports(self.project.get(), "hub")[1]  # monitor_port of the hub env
        if not port:
            messagebox.showwarning("OTA", "Chọn cổng COM của hub.")
        return port

    def trigger_hub_fw(self):
        if self.running:
            return
        port = self._hub_port()
        if not port:
            return
        dry = self.ota_dry.get()
        if not dry:
            if not messagebox.askokcancel("CẢNH BÁO - Cập nhật firmware hub", OTA_FW_WARNING, icon="warning"):
                return
            self.write("WARNING: " + OTA_FW_WARNING.replace("\n\n", "\n").replace("\n", " | ") + "\n", "warn")
        self.close_monitors(port)
        self._set_busy(True, "Hub firmware OTA...")
        threading.Thread(target=self._ota_serial, args=(port, "FWOTA,DRY" if dry else "FWOTA"), daemon=True).start()

    def trigger_ota(self):
        if self.running:
            return
        if not self.ota_server:
            self.start_ota_server()
            if not self.ota_server:
                return
        project = self.project.get()
        port = self.port.get()
        if port in ("", AUTO_PORT, "(auto)"):
            port = ini_ports(project, "hub")[1]  # monitor_port of the hub env
        if not port:
            messagebox.showwarning("OTA", "Chọn cổng COM của hub.")
            return
        if not self.ota_dry.get():
            if not messagebox.askokcancel("CẢNH BÁO - Cập nhật màn hình", OTA_WARNING, icon="warning"):
                return
            self.write("WARNING: " + OTA_WARNING.replace("\n\n", "\n").replace("\n", " | ") + "\n", "warn")
        command = f"TFTOTA,{self._ota_file_url()}" + (",DRY" if self.ota_dry.get() else "")
        self.close_monitors(port)
        self._set_busy(True, "HMI OTA...")
        threading.Thread(target=self._ota_serial, args=(port, command), daemon=True).start()

    def _ota_serial(self, port, command):
        """Send the command over USB serial, release the port, then reopen the log console on it."""
        put = self.lines.put
        code = 1
        try:
            try:
                import serial
            except ImportError:
                put(("Installing pyserial...\n", "cmd"))
                subprocess.run([sys.executable, "-m", "pip", "install", "pyserial"],
                               capture_output=True, check=True)
                import serial
            link = serial.Serial()
            link.port, link.baudrate, link.timeout = port, 115200, 0.5
            link.dtr = link.rts = False  # opening the port must not reset the hub
            link.open()
            with link:
                put((f"$ {port} <- {command}\n", "cmd"))
                link.write((command + "\n").encode())
                link.flush()
                time.sleep(0.3)
            code = 0
            put(("Command sent. The hub log continues in the monitor console.\n", "ok"))
        except Exception as exc:
            put((f"{exc}\n", "err"))
        self.last_code = code
        self.on_done = lambda: self._open_serial_monitor(port)
        put(None)

    def _open_serial_monitor(self, port):
        """Open the log console on `port` without toggling DTR/RTS, so the running update is not reset."""
        mon = self.pio + ["device", "monitor", "-d", self.project.get(), "-e", "hub",
                          "--port", port, "--dtr", "0", "--rts", "0"]
        self.write("$ " + " ".join(mon) + "\n", "cmd")
        self.monitors[port] = subprocess.Popen(
            mon, creationflags=subprocess.CREATE_NEW_CONSOLE if IS_WIN else 0)

    def stop(self):
        for proc in list(self.procs):
            proc.terminate()

    def _set_busy(self, busy, text, color=None):
        self.running = busy
        self.dot.itemconfigure(self.dot_item, fill=THEME["warn"] if busy else (color or THEME["ok"]))
        if busy:
            self.progress.pack(fill="x", after=self.status_badge.master)
            self.progress.start(12)
        else:
            self.progress.stop()
            self.progress.pack_forget()

        for b in self.buttons:
            b.set_state("disabled" if busy else "normal")
        for b in self.log_buttons:
            b.set_state("disabled" if busy else "normal")
        self.stop_btn.set_state("normal" if busy else "disabled")

        self.status.configure(text=text)


if __name__ == "__main__":
    App(sys.argv[1] if len(sys.argv) > 1 else ".").mainloop()