# Unix-Shell

A Unix-like shell built from scratch in C++ with pipelines, job control, I/O redirection, persistent history, and tab completion.

## Features

- **Command execution** — runs external commands via `fork()` + `execvp()`
- **Pipelines** — two-command pipes using `pipe()`, `fork()`, and `dup2()`
- **Background jobs** — run commands with `&`, track with a `jobs` builtin
- **Job control** — job number recycling, async reaping via `waitpid(WNOHANG)`
- **I/O redirection** — `>`, `<`, `>>` support
- **Shell history** — persistent history with HISTFILE, `history`, `history -r`, `history -w`, `history -a`
- **Tab completion** — custom TAB completion using `complete -C` external completer scripts with longest-common-prefix logic and bell-on-first / list-on-second behavior
- **Variable expansion** — `declare` builtin and `expandVars()` for parameter expansion
- **GNU Readline** — full readline integration for line editing

## Builtins

| Command | Description |
|---|---|
| `cd` | Change directory |
| `pwd` | Print working directory |
| `echo` | Print arguments |
| `export` | Set environment variables |
| `declare` | Declare shell variables |
| `jobs` | List background jobs |
| `history` | Show/manage command history |
| `exit` | Exit the shell |

## Installation

**Requirements:** Linux, g++, GNU Readline

```bash
# Clone the repo
git clone https://github.com/vinaayy06/shell.git
cd shell

# Install readline (if not already installed)
sudo apt-get install libreadline-dev

# Build
g++ -std=c++17 src/*.cpp -lreadline -o shell

# Run
./shell
```

## Usage

```bash
# Basic command
$ ls -la

# Pipeline
$ ls | grep .cpp

# Background job
$ sleep 10 &
[1] 12345

# Check jobs
$ jobs
[1]+ Running    sleep 10

# Redirect output
$ echo hello > out.txt

# Variable declaration
$ declare x=42
$ echo $x
42

# History
$ history
$ history -w   # write to HISTFILE
$ history -r   # read from HISTFILE
```

## Technical Details

- Process management: `fork()`, `exec()`, `waitpid()`
- IPC: `pipe()`, `dup2()` for pipeline chaining
- Signal handling: background job reaping with `WNOHANG`
- Readline integration via GNU Readline library
- Tab completion via external completer scripts (`complete -C`)

## Languages

C++ 100%
