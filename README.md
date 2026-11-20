# NVtopps Log Disabler

[Deutsche README](README.de.md)

Small Windows tool to disable or re-enable the faulty `_nvtopps` logging of the NVIDIA Container
LocalSystem, which can wear out the SSD.

**English and German UI:** if the Windows display language is German, the app shows German;
every other language shows English.

**Important:** repeat the procedure after every NVIDIA driver update, because the registry value
can be reset during the update.

Based on this video: https://www.youtube.com/watch?v=W6m5bIpq__Y

## What it does

It only reads and writes this value:

```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NVDisplay.ContainerLocalSystem\Parameters\LocalSystem\_nvtopps
DisableLoad (REG_DWORD)
```

| Value | Meaning |
| --- | --- |
| `1` | Logging disabled |
| `0` | Logging enabled |
| Key/value missing | Logging enabled (default) |

If the key is missing, it is created automatically on the first change. After every change the app
asks whether to restart the PC now — the change only takes effect after the restart. A restart can
also be triggered later with the **Restart PC** button.

The app requires **administrator rights** (requested in the manifest), because `HKEY_LOCAL_MACHINE`
cannot be written otherwise.

The two `.reg` files in the repository are reference material only and are not part of the program.

## Build

The GitHub workflow `.github/workflows/build.yml` builds the exe with MSVC (`/O1 /GL /MT`, static CRT,
no additional runtime installer needed) on every push to `main`, on pull requests and on tags.

Locally, in a Visual Studio Developer Prompt:

```bat
cl /nologo /std:c11 /O1 /GL /MT /utf-8 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /c src\main.c /Fobuild\main.obj
rc /nologo /I src /fo build\resource.res src\resource.rc
link /nologo /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /MANIFEST:NO /OUT:build\nvtopps-log-disabler.exe build\main.obj build\resource.res kernel32.lib user32.lib gdi32.lib advapi32.lib
```

## Release

Just push a version tag — the workflow compiles, packages the exe with both READMEs and LICENSE and
publishes the zip as a GitHub Release:

```bash
git tag v1.0.1
git push origin v1.0.1
```

---

# Deutsch

[English README](README.md)

Kleines Windows-Tool, um die fehlerhafte `_nvtopps`-Protokollierung des NVIDIA Container LocalSystem
abzuschalten oder wieder zu aktivieren. Der SSD-Verschleiß durch das fehlerhafte Logging wird damit gestoppt.

Die App ist **englisch und deutsch**: Ist die Windows-Anzeigesprache Deutsch, erscheint sie auf Deutsch,
bei jeder anderen Sprache auf Englisch.

**Wichtig:** Nach jedem NVIDIA-Treiber-Update muss der Vorgang wiederholt werden, da der Registry-Wert
dabei zurückgesetzt werden kann.

Basiert auf diesem Video: https://www.youtube.com/watch?v=W6m5bIpq__Y

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

Einfach einen Versionstag pushen — der Workflow kompiliert, packt die exe zusammen mit den READMEs
und LICENSE und veröffentlicht das Zip als GitHub Release:

```bash
git tag v1.0.1
git push origin v1.0.1
```
