# Shell

I never really thought about what happens when you type a 
command and hit enter. It just... works. Until you try to 
build one yourself.

Took up the CodeCrafters shell challenge to find out. Ended 
up spending weeks inside fork(), exec(), pipes, and file 
descriptors — the stuff that's been running silently under 
every terminal session I've ever had.

## What it does

- Executes external commands
- Pipes two commands together — `ls | grep main` actually works
- Handles input/output redirection (`>`, `<`, `>>`)
- Runs commands in the background with `&`
- Tracks background jobs and cleans them up properly
- Persists command history across sessions
- TAB completion with longest-common-prefix logic
- Variable declaration and expansion via a `declare` builtin

## Build

```bash
# You'll need GNU Readline
sudo apt install libreadline-dev

# Compile
g++ -o shell src/main.cpp -lreadline

# Run
./shell
```

## Honest reflection

The hardest parts weren't the features. It was debugging 
what I didn't know I didn't know — zombie processes, broken 
pipes, completion edge cases. Those bugs taught me more than 
any lecture did.

Still building. HTTP server is next.
