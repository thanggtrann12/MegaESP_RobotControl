import io
import re
import glob

for f in sorted(glob.glob('doc/locales/vi/LC_MESSAGES/*.po')):
    text = io.open(f, encoding='utf-8').read()
    entries = re.findall(r'(?ms)^msgid ((?:"[^"]*"\n)+)msgstr ((?:"[^"]*"\n)*)', text)
    total = 0
    untranslated = []
    for mid, mstr in entries:
        mid_s = ''.join(re.findall(r'"([^"]*)"', mid))
        if not mid_s:
            continue  # header
        total += 1
        mstr_s = ''.join(re.findall(r'"([^"]*)"', mstr))
        if not mstr_s.strip():
            untranslated.append(mid_s[:70].replace('\n', ' '))
    print('=== %s: %d entries, %d untranslated' % (f, total, len(untranslated)))
    for u in untranslated:
        print('   -', u)
