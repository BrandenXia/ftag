# ftag

A tagging command line tool for managing files and directories. Design to support
remote file systems and to be integrated with other programs.

## Installation

### via Makefile

```bash
MODE=RELEASE make install
```

### Manually

```bash
MODE=RELEASE make
install -D build/ftag /usr/local/bin/ftag
# optional: zsh completions
install -D completions/_ftag /usr/local/share/zsh/site-functions/_ftag
```

### Uninstall

```bash
make uninstall
```
