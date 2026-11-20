# NVtopps Log Disabler

Kleines Windows-Tool, um die fehlerhafte `_nvtopps`-Protokollierung des NVIDIA Container LocalSystem
abzuschalten oder wieder zu aktivieren. Der SSD-Verschleiß durch das fehlerhafte Logging wird damit gestoppt.

Die App ist **englisch und deutsch**: Ist die Windows-Anzeigesprache Deutsch, erscheint sie auf Deutsch,
bei jeder anderen Sprache auf Englisch.

**Wichtig:** Nach jedem NVIDIA-Treiber-Update muss der Vorgang wiederholt werden, da der Registry-Wert
dabei zurückgesetzt werden kann.

Basiert auf diesem Video: https://www.youtube.com/watch?v=W6m5bIpq__Yen

## Was sie macht

Sie liest und schreibt ausschließlich diesen Wert:

```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NVDisplay.ContainerLocalSystem\Parameters\LocalSystem\_nvtopps
DisableLoad (REG_DWORD)
```

| Wert | Bedeutung |
| --- | --- |
| `1` | Logging deaktiviert |
| `0` | Logging aktiv |
| Schlüssel/Wert fehlt | Logging aktiv (Standard) |

Fehlt der Schlüssel, wird er beim ersten Umschalten automatisch angelegt. Nach jedem Umschalten
fragt die App, ob der PC jetzt neu gestartet werden soll — erst nach dem Neustart greift die Änderung.
Ein Neustart kann später auch über die Schaltfläche **PC neu starten** ausgelöst werden.

Die App benötigt **Administratorrechte** (im Manifest angefordert), weil `HKEY_LOCAL_MACHINE`
sonst nicht geschrieben werden kann.

Die beiden `.reg`-Dateien im Repository dienen nur als Referenz und sind nicht Teil des Programms.

## Build

Der GitHub-Workflow `.github/workflows/build.yml` baut die exe mit MSVC (`/O1 /GL /MT`, statische CRT,
kein zusätzliches Runtime-Installer nötig) und läuft bei Push auf `main`, bei Pull Requests und bei Tags.

Lokal mit Visual Studio Developer Prompt:

```bat
cl /nologo /std:c11 /O1 /GL /MT /utf-8 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /c src\main.c /Fobuild\main.obj
rc /nologo /I src /fo build\resource.res src\resource.rc
link /nologo /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /MANIFEST:NO /OUT:build\nvtopps-log-disabler.exe build\main.obj build\resource.res kernel32.lib user32.lib gdi32.lib advapi32.lib
```

## Release

Einfach einen Versionstag pushen — der Workflow kompiliert, packt die exe zusammen mit README
und LICENSE und veröffentlicht das Zip als GitHub Release:

```bash
git tag v1.0.0
git push origin v1.0.0
```

---

# English

Small Windows tool to disable or re-enable the faulty `_nvtopps` logging of the NVIDIA Container
LocalSystem, which can wear out the SSD.

**English and German UI:** if the Windows display language is German, the app shows German;
every other language shows English.

**Important:** repeat the procedure after every NVIDIA driver update, because the registry value
can be reset during the update.

Based on this video: https://www.youtube.com/watch?v=W6m5bIpq__Yen

It only reads/writes `DisableLoad` (REG_DWORD) under
`HKLM\SYSTEM\CurrentControlSet\Services\NVDisplay.ContainerLocalSystem\Parameters\LocalSystem\_nvtopps`.
`1` = logging off, `0` = logging on. A missing key/value means logging is on and the key is created
on the first change. After every change the app asks whether to restart now; the change takes effect
after the restart. **Administrator rights** are required and requested via the manifest.

The two `.reg` files are reference material only and are not part of the app.

Builds and releases are automated in `.github/workflows/build.yml`. Push a `v*` tag to publish a
GitHub Release.
