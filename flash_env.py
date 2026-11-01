#!/usr/bin/env python3
"""Tkinter GUI for PlatformIO - Modern Dashboard Style.

Run:  python flash_env.py [project_dir]
"""

import configparser
import json
import queue
import shutil
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
        self.minsize(820, 650)

        # 1. Header Section
        header = ttk.Frame(self, padding=(20, 16, 20, 8))
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

        self.progress = ttk.Progressbar(self, mode="indeterminate", style="Busy.Horizontal.TProgressbar")

        # 2. Project Card Section (Giống mẫu)
        proj_outer = tk.Frame(self, bg=THEME["border"])
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

        # 3. Targets Header Bar & Danh sách môi trường
        targets_bar = ttk.Frame(self, padding=(20, 8, 20, 4))
        targets_bar.pack(fill="x")

        self.targets_count_lbl = ttk.Label(targets_bar, text="0 firmware targets", font=(FONT[0], 10, "bold"))
        self.targets_count_lbl.pack(side="left")

        # Nút Build All và Clean chung ở góc phải
        btn_clean_all = ModernButton(targets_bar, text="Clean", command=lambda: self.run_pio(None, "clean"),
                                     tip="Xóa kết quả build của tất cả môi trường")
        btn_clean_all.pack(side="right", padx=(6, 0))
        self.buttons.append(btn_clean_all)

        btn_build_all = ModernButton(targets_bar, text="Build All", variant="accent",
                                     command=lambda: self.run_pio(None, "build"),
                                     tip="Build tất cả môi trường")
        btn_build_all.pack(side="right")
        self.buttons.append(btn_build_all)

        btn_upload_sel = ModernButton(targets_bar, text="⬆ Upload đã chọn", variant="accent",
                                      command=lambda: self.run_selected("upload"),
                                      tip="Flash các target đã tick cùng lúc (cổng khác nhau chạy song song)")
        btn_upload_sel.pack(side="right", padx=(0, 6))
        self.buttons.append(btn_upload_sel)

        btn_build_sel = ModernButton(targets_bar, text="⚙ Build đã chọn",
                                     command=lambda: self.run_selected("build"),
                                     tip="Build các target đã tick cùng lúc")
        btn_build_sel.pack(side="right", padx=(0, 6))
        self.buttons.append(btn_build_sel)

        btn_select_all = ModernButton(targets_bar, text="☑ Chọn tất cả", command=self.toggle_all,
                                      tip="Tick hoặc bỏ tick tất cả target")
        btn_select_all.pack(side="right", padx=(0, 6))
        self.buttons.append(btn_select_all)
        self.top_buttons = list(self.buttons)

        # Vùng chứa các card envs động
        self.env_frame = ttk.Frame(self, padding=(20, 2))
        self.env_frame.pack(fill="x")

        # 4. Activity Log Section (Đúng chuẩn mẫu)
        log_header = ttk.Frame(self, padding=(20, 10, 20, 4))
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

        log_container = tk.Frame(self, bg=THEME["border"])
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
            lbl.pack(anchor="w", pady=5)
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
        outer.pack(fill="x", pady=4)

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
        try:
            out = subprocess.run(self.pio + ["device", "list", "--json-output"],
                                 capture_output=True, text=True, timeout=20).stdout
            ports = [d["port"] for d in json.loads(out)]
        except Exception:
            ports = []
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