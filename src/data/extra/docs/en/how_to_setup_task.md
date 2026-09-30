# VNote Task Configuration Guide

VNote ships with a simple **task** system, designed after [VSCode Tasks](https://code.visualstudio.com/docs/editor/tasks), which lets you run third-party programs and scripts from inside VNote.

Use it to compile the current note, start a local server, open files in another editor, drive git, or anything else you can write a script for.

Tasks are defined in **JSON configuration files**, and the output is shown in VNote's **Console** panel.

> This guide is based on the current code (`src/task/`, `src/widgets/toolbarhelper.cpp`, `src/snippet/snippetmgr.cpp`). Compared with the earlier versions of the document, the search folders, the reload flow, the variable names and the magic-word list have all changed - this document is authoritative.



## Quick Start

###  Hello World

1. Click the **Task** menu on the toolbar → **Add Task** → **To User Folder**. The user task folder will be opened:

   ```
   C:\Users\<user>\AppData\Local\VNote\VNote\tasks
   ```

   (The folder is created automatically if it does not exist. In portable mode it is `<VNote folder>\user_files\tasks`.)

2. Create `hello.json` in that folder:

   ```json
   {
       "command": "echo helloworld"
   }
   ```

3. Back in VNote, click **Task** → **Reload** (no restart is needed after changing the configuration). A `hello` entry now appears in the menu; click it to run.

Running a task automatically activates the **Console** panel at the bottom, where you can see the output. By default the command is handed over to the system shell ( `PowerShell.exe` on Windows, `/bin/bash` on Linux/macOS).

###  Menu Name, Icon and Shortcut

By default a task is shown in the menu with its **file name** (top level task) or its `command` (child task). Use `label`, `icon` and `shortcut` to customize it:

```json
{
    "label": "Hello",
    "icon": "tasks-solid.svg",
    "shortcut": "Alt+H, T",
    "command": "echo",
    "args": ["Hello tasks!"]
}
```

- `icon`: SVG is recommended (VNote recolors it according to the current theme). The path may be **absolute** or **relative to the folder of the task file**; if the file does not exist the icon is ignored.
- `shortcut`: a shortcut such as `Alt+H, T`, registered on the menu entry; pressing it runs the task.
- `&` inside `label` is escaped automatically and is not treated as a shortcut prefix, so it is safe to use.

###  Child Tasks

Tasks can be nested to any depth and are shown as multi-level sub-menus:

```json
{
    "label": "Hello Tasks",
    "icon": "tasks-solid.svg",
    "command": "echo",
    "args": ["Hello tasks!"],
    "tasks": [
        { "label": "Hello Cat",  "icon": "cat-solid.svg",  "shortcut": "Alt+H, C", "args": ["Hello cat!"] },
        { "label": "Hello Dove", "icon": "dove-solid.svg", "shortcut": "Alt+H, D", "args": ["Hello dove!"] },
        { "label": "Hello Fish", "icon": "fish-solid.svg", "shortcut": "Alt+H, F", "args": ["Hello fish!"] }
    ]
}
```

A child task **inherits** `version`, `type`, `command`, `args`, `options.cwd`, `options.env` and `options.shell` from its parent; `label`, `icon`, `shortcut`, `inputs` and `tasks` are not inherited (a child task without a `label` is displayed as its `command`).

> Note: if a task has child tasks, clicking its menu entry only expands the sub-menu. To run the parent's own command, give it a `shortcut` (or simply do not configure a command on that parent).

### Running an External Program

Set `type` to `process` and `command` is started directly as an **executable path** (without a shell):

```json
{
    "type": "process",
    "label": "Open File with",
    "args": ["${buffer}"],
    "tasks": [
        { "label": "Typora",  "icon": "Typora.svg", "command": "C:\\Programs\\Typora\\Typora.exe" },
        { "label": "VS Code", "icon": "vscode.svg", "command": "C:\\Users\\me\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe" }
    ]
}
```

Here `${buffer}` is the path of the file currently being edited/viewed (see section 5).

VNote has no console window of its own, so for interactive command line programs it is better to let a shell open a separate terminal window. For example `start cmd "/c ..."` on Windows, or `gnome-terminal --execute ...` on Linux:

```json
{
    "label": "Vim",
    "icon": "vim.svg",
    "type": "process",
    "command": "gnome-terminal",
    "args": ["--execute", "vim", "${buffer}"]
}
```

###  Multiple Platforms and Languages

**Multiple platforms**: use the `windows`, `linux` and `osx` keys for platform specific configuration. Platform specific configuration **overrides** the base configuration, except that the `tasks` array is **merged (appended)**.

```json
{
    "type": "process",
    "label": "Open File with",
    "args": ["${buffer}"],
    "tasks": [
        {
            "label": "Typora",
            "icon": "Typora.svg",
            "windows": { "command": "C:\\Programs\\Typora\\Typora.exe" },
            "linux":   { "command": "/usr/bin/typora" }
        },
        {
            "label": "VS Code",
            "icon": "vscode.svg",
            "windows": { "command": "C:\\Users\\me\\AppData\\Local\\Programs\\Microsoft VS Code\\Code.exe" },
            "linux":   { "command": "/usr/bin/code" }
        }
    ]
}
```

**Multiple languages**: the "translatable" fields - `label`, `command`, elements of `args`, the values of `options.env`, and `description`/`default`/`options` of `inputs` - accept either a plain string or an object of the form `{ "locale": "value" }`. When the current language is missing, the first entry of the object is used.

```json
{
    "label": { "en_US": "Hello", "zh_CN": "你好" },
    "command": "echo",
    "tasks": [
        {
            "label": { "en_US": "Cat", "zh_CN": "猫" },
            "args": [{ "en_US": "Hello cat!", "zh_CN": "你好，猫！" }]
        }
    ]
}
```

The current language is the UI language of VNote (changing it in the settings requires a restart).



## Task Folders and Menu Structure

###  The Three Levels

| Level | Folder | Description |
| --- | --- | --- |
| Built-in tasks | `<app config folder>/tasks` | Shipped with the program and copied automatically when the configuration version is updated. The path is `%APPDATA%\VNote\VNote\tasks`; in portable mode `<VNote folder>\vnotex_files\tasks`. Currently the only built-in group is **Git**. |
| User tasks | `<user config folder>/tasks` | Added by the user; the folder is created automatically if it does not exist. The path is `%LOCALAPPDATA%\VNote\VNote\tasks`; in portable mode `<VNote folder>\user_files\tasks`. |
| Notebook tasks | `<notebook root>/vx_notebook/tasks` | Only effective for the **current notebook**; you have to create this folder yourself. Useful for tasks that only one notebook needs (for example publishing a site). |

- Every `*.json` file in the folders above **and in their sub-folders** is loaded as a task configuration file (each file corresponds to one group of tasks in the menu).
- **When they are loaded**: everything is loaded at startup; notebook tasks are reloaded when you switch notebooks; after editing a configuration click **Task → Reload** - **restarting VNote is not required**.

###  Menu Structure

```
Task ▾
 ├─ Add Task ▸
 │   ├─ To User Folder       opens the user task folder (created automatically if missing)
 │   └─ To Current Notebook Folder  opens <notebook>/vx_notebook/tasks (created automatically if missing;
 │                                  greyed out when there is no current notebook)
 ├─ Reload
 ├─────────────
 │  (Built-in tasks)
 ├─────────────
 │  User Tasks            ← the group label is only shown when both "User Tasks" and
 │  ...                     "Notebook Tasks" provide tasks
 │  Notebook Tasks
 │  ...
```

In other words: when both the **User Tasks** and the **Notebook Tasks** groups contain tasks, a label naming the source folder is inserted so that tasks with the same name can be told apart; if only one of them provides tasks, no label is added.



## Configuration Reference

A task configuration file is simply a JSON object (think of it as "one group of tasks"). Every key is optional.

| Key | Type | Description |
| --- | --- | --- |
| `version` | string | Configuration version, default `0.1.3`. The current code only accepts **0.x** versions; `1.0.0` or higher is treated as an unknown version and the task is **ignored**. Usually you do not need to write it. |
| `label` | translatable string | Task name. Defaults to the file name for a top level task and to `command` for a child task. |
| `icon` | string | Icon path, relative to the **folder of the task file** or absolute; the file must exist. |
| `shortcut` | string | Shortcut, e.g. `Alt+H, T`. |
| `type` | string | `shell` (default) or `process`. |
| `command` | translatable string | The task command. Empty means "do not run" (only used as a menu group). |
| `args` | translatable string array | Command arguments. |
| `options` | object | Run options, see below. |
| `tasks` | array | Child tasks, nested to any depth. |
| `inputs` | array | Input variable definitions, see below. |
| `messages` | array | Reserved; the current version **parses it but does not use it**. |
| `windows`/`linux`/`osx` | object | Platform specific configuration, overriding the base values (its `tasks` are appended). |

### `options`

| Key | Description |
| --- | --- |
| `cwd` | Working directory. When missing, the following are tried in order: **current notebook root** → **folder of the current file** → **folder of the task configuration file**. |
| `env` | Object of extra/overriding environment variables (values are translatable). Note: **these values are not variable-substituted**. |
| `shell.executable` | The shell executable; only read when `type` is `shell`. |
| `shell.args` | Shell startup arguments; when missing, the defaults listed below are used. |

### `inputs`

| Key | Description |
| --- | --- |
| `id` | Required; referenced by `${input:id}`. |
| `type` | `promptString` (default, shows an input box) or `pickString` (shows a selection box). |
| `description` | Prompt text (translatable, supports variables). |
| `default` | Default value (translatable, supports variables). With `pickString` this value must appear in `options`. |
| `password` | Boolean, only effective for `promptString`, for password input boxes. |
| `options` | String array, only effective for `pickString`, the list of choices (translatable). |



## Shell and Special Characters

- **Default shell**: `PowerShell.exe` on Windows, `/bin/bash` on Linux/macOS.
- **Default startup arguments**:

  | Shell | Default arguments |
  | --- | --- |
  | `cmd.exe` | `/C` |
  | `PowerShell.exe` | `-Command` |
  | `/bin/bash` | `-c` |

  Other shells get no arguments by default; specify them yourself via `options.shell.args`.

- **How the command is assembled** (`src/task/shellexecution.cpp`):
  - When the **base name of the shell is `bash`** (including the default `/bin/bash`): `command` and every element of `args` that contains a space is wrapped in double quotes, then everything is joined with spaces into **a single string** and passed to bash as the single argument of `-c`.
  - For other shells (PowerShell, cmd, ... on Windows): `args` are passed to the process as **separate arguments** and quoting is handled by the system, so you do not need to add quotes manually.
  - This is why the same task may need different spellings on different platforms, which is exactly what the `windows`/`linux` platform configuration is for.

- When a task **needs interaction or must keep running**, let the shell open a terminal window (see the example in 1.4); VNote itself does not take over standard input.



## Variable Substitution

The syntax is `${variableName}` and it can be used in **`command`, `args`, `options.cwd` and `options.shell.args`** (the `description`/`default` of `inputs` also support variables). When no file is open, no notebook is loaded, and so on, variables without a value are replaced by an **empty string**. Path-like variables use the **platform specific** separator (`\` on Windows).

### Predefined Variables

Assume:

- The current notebook is named `test-task` with the root folder `C:\notes\test-task`
- The current file is `C:\notes\test-task\test2\note.md`
- The current selection is `a test`

Then:

| Variable | Meaning | Example value |
| --- | --- | --- |
| `${notebookFolder}` | Root folder of the current notebook | `C:\notes\test-task` |
| `${notebookFolderName}` | Folder name of the current notebook | `test-task` |
| `${notebookName}` | Name of the current notebook | `test-task` |
| `${notebookDescription}` | Description of the current notebook | `This notebook for task test.` |
| `${buffer}` | Path of the file being edited/viewed | `C:\notes\test-task\test2\note.md` |
| `${bufferNotebookFolder}` | Notebook root of the current file | `C:\notes\test-task` |
| `${bufferRelativePath}` | Path of the current file relative to the notebook root | `test2\note.md` |
| `${bufferName}` | File name of the current file | `note.md` |
| `${bufferBaseName}` | File name without extension | `note` |
| `${bufferDir}` | Folder of the current file | `C:\notes\test-task\test2` |
| `${bufferExt}` | Extension of the current file (without the dot) | `md` |
| `${selectedText}` | Currently selected text | `a test` |
| `${cwd}` | Working directory of this run (see the `options.cwd` rules) | `C:\notes\test-task` |
| `${taskFile}` | Path of the configuration file of the current task | |
| `${taskDir}` | Folder of the configuration file of the current task | |
| `${exeFile}` | Path of the VNote executable | `C:\Programs\vnote3\vnote.exe` |
| `${pathSeparator}` | Path separator of the current platform | `\` |
| `${notebookTaskFolder}` | Task folder of the current notebook | `C:\notes\test-task\vx_notebook\tasks` |
| `${userTaskFolder}` | User task folder | `C:\Users\me\AppData\Local\VNote\VNote\tasks` |
| `${appTaskFolder}` | Built-in (app) task folder | `C:\Users\me\AppData\Roaming\VNote\VNote\tasks` |
| `${userThemeFolder}` / `${appThemeFolder}` | User/built-in theme folders | |
| `${userDocsFolder}` / `${appDocsFolder}` | User/built-in document folders | |

> In earlier versions `${file}`, `${fileBasename}` and friends were renamed to the `${buffer*}` family; `${execPath}` is now `${exeFile}`, `${taskDirname}` is now `${taskDir}`, and `${notebookFolderBasename}` is now `${notebookFolderName}`. The current version has **no** `${lineNumber}`.

###  Magic Words (magic)

`${magic:name}` actually reuses VNote's **snippet** mechanism: `${magic:date}` is turned into the snippet symbol `%date%` and expanded.

Built-in snippet names (see `SnippetMgr::loadBuiltInSnippets`):

| Category | Names |
| --- | --- |
| Day/week/month/year | `d`, `dd`, `ddd`, `dddd`, `M`, `MM`, `MMM`, `MMMM`, `yy`, `yyyy`, `w`, `ww` |
| Hour/minute/second | `H`, `HH`, `m`, `mm`, `s`, `ss` |
| Combinations | `date` (`2021-02-24`), `da` (`20210224`), `time` (`16:51:02`), `datetime` (`2021-02-24_16:51:02`) |
| Current note | `note` (note file name), `no` (note name without extension) |

For example `${magic:datetime}` → `2021-02-24_16:51:02`.

**You can define your own magic words**: put your own snippet JSON files under `<user config folder>/snippets/` (the same snippets used as `%name%` in the editor) and you can reference them through `${magic:your-name}`; a name that clashes with a built-in one is skipped.

### Environment Variables

`${env:name}` reads a system environment variable, for example:

- `${env:ComSpec}` → `C:\Windows\system32\cmd.exe`
- `${env:TEMP}` → `C:\Users\me\AppData\Local\Temp`

###  Configuration Variables

`${config:path}` reads a value from the VNote configuration file (`vnotex.json`), for example `${config:core.locale}` or `${config:metadata.version}`.

Only the `object.key` and `array[index]` forms are supported, and only strings/numbers/booleans are returned (booleans become `1`/`0`; arrays and objects return an empty string).

###  Input Variables

`${input:id}` refers to an entry defined in `inputs` and asks for it in a dialog at run time:

```json
{
    "command": "echo",
    "args": ["${input:what}"],
    "inputs": [
        {
            "id": "what",
            "type": "promptString",
            "description": "Type something, it will show in console panel."
        }
    ]
}
```

A `pickString` example:

```json
{
    "command": "echo",
    "args": ["${input:who}"],
    "inputs": [
        {
            "id": "who",
            "type": "pickString",
            "description": "Choose a target",
            "options": ["cat", "dove", "fish"],
            "default": "cat"
        }
    ]
}
```

The same input variable appearing several times in one field is only asked once; cancelling the input **aborts the whole task**.

### Shell Variables

`${shell:command}` runs the command with the default shell and uses its **standard output** as the value, for example:

- `${shell:git rev-parse --abbrev-ref HEAD}` → `main`
- `${shell:whoami}` → `me`

The working directory is the same as that of the task, and a command that takes longer than 1 second aborts the task.

###  Evaluation Rules

- Variables may appear nested (if the substituted text still contains variables it is evaluated again), but a **variable name** itself cannot be nested, as in `${a${b}}`.
- Paths inside variable values use the platform specific separator.
- If the value of `${magic:*}` or `${shell:*}` contains comments or line breaks, mind the shell syntax yourself.



## Output and Errors

- Task output (stdout/stderr) is collected in the **Console** dock panel, which is activated automatically when a task runs. The panel tries to decode the output with `UTF-8` → `System` → `UTF-16` → `GB18030` in order, and falls back to the local 8-bit encoding if all of them fail.
- Three lines are inserted into the output while running:

```
[Task (Hello) started]
...output of the task...
[Task (Hello) finished (0)]
```

On failure it prints:

```
[Task (Hello) error occurred (0)]
```

The number in parentheses is the Qt process error code:

| Error code |                                Meaning                                |
| ---------- | --------------------------------------------------------------------- |
| 0          | Failed to start (bad command or arguments, executable not found, ...) |
| 1          | The process crashed                                                   |
| 2          | Timed out                                                             |
| 3          | Write error                                                           |
| 4          | Read error                                                            |
| 5          | Unknown error                                                         |

- The current version does **not** support typing into a task process from the Console (still a TODO in the code). For interactive programs, open a terminal window as described in 1.4.



## Examples

### Git (Built-in Tasks)

After **Task → Reload** you will find the built-in `Git` group at the bottom of the menu, containing:

| Menu entry | Equivalent command |
| --- | --- |
| Initialize | `git init -b main` |
| Status | `git status` |
| Commit | `git add -A -- . && git commit --message="${input:msg}"` (on Windows: `; if ($?) { ... }`) |
| Push | `git push` |
| Pull | `git pull --no-rebase` |
| Log | `git log -10 --graph --pretty=format:'%h -%d %s (%cr) <%an>' --abbrev-commit` |

- "Commit" asks for the commit message in a dialog; the default value is `Update note at ${magic:datetime}` (the Chinese UI uses `更新笔记于 ${magic:datetime}`).
- The working directory defaults to the **current notebook root** (the `options.cwd` fallback rules), so switch to the target notebook first.
- Tip: by default Git shows non-ASCII file names as escape sequences; run `git config --global core.quotepath false` to turn that off.

###  A Task for One Notebook Only (Notebook Tasks)

Put `publish.json` under `<notebook>/vx_notebook/tasks`, for example to publish a site:

```json
{
    "label": "Publish",
    "command": "hexo clean && hexo generate && hexo deploy",
    "options": { "cwd": "${shell:git rev-parse --show-toplevel}" }
}
```

(That folder can be opened quickly via **Task → Add Task → To Current Notebook Folder**.)

###  Interactive Program: Compile and Run in a New Window

```json
{
    "command": "g++ \"${buffer}\" -o \"${bufferBaseName}\"; if ($?) { start cmd \"/c `\"${bufferBaseName}`\" & pause\" }"
}
```

Note: do **not** split such long commands into `command` + `args`; the quoting rules differ a lot between shells (see section 4).

###  Background Service: an HTTP Server in the Notebook Folder

```json
{
    "command": "start cmd.exe \"/c python -m http.server\" ; start http://localhost:8000"
}
```



## Not Implemented / Differences from VSCode Tasks

The following configuration items exist in VSCode but are **not implemented** in the current VNote version (writing them has no effect):

- Task dependencies: `dependsOn`, `dependsOrder`
- Output control: `presentation` (`reveal`/`clear`, ...)
- Automatic triggering: `runOptions.runOn` (run when a notebook is opened/closed, when VNote starts/quits)
- Task `id` and `group`
- The `messages` configuration: it is parsed, but not used yet
- Running tasks from the "united entry"/command palette (only the menu and shortcuts are available)
- A graphical editor for task configuration (JSON only)
- Typing into a process from the Console

Apart from those, `type` (`shell`/`process`), `command`/`args`, variables, input variables, and platform/language overrides are basically compatible with VSCode's syntax.



## FAQ

**Q: I wrote the JSON but nothing shows up in the menu.**

Check in order:

1. Is the file inside one of the three task folders (or their sub-folders) and does it end with `.json`?
2. Did you click **Task → Reload**?
3. If it is in `<notebook>/vx_notebook/tasks`, you must first **open/switch to that notebook**.
4. If `version` is `1.0.0` or higher the task is ignored (the log says `unknown task version`).
5. JSON syntax errors, an `icon` pointing to a missing file and so on leave warnings in the log (`<user config folder>/vnotex.log`).

**Q: Why does a child task run even though I did not write a `command` for it?**

Child tasks inherit `type`/`command`/`args`/`options` from their parent (see 1.3).

**Q: Why does clicking a menu entry that has child tasks do nothing?**

It expands the sub-menu; run the child tasks or the parent's own command through their own menu entries or shortcuts.

**Q: The output looks like garbage.**

First check the encoding in the Console; VNote tries `UTF-8 → System → UTF-16 → GB18030`, so usually you do not need to do anything. If a tool outputs GBK, one of those still matches.

**Q: `${buffer}` is empty.**

It has no value when no file is open; likewise the `${notebookFolder}` family is empty when no notebook is loaded.

**Q: What about paths containing spaces?**

With `type: shell` and a bash shell, VNote automatically quotes arguments that contain spaces; for other shells (PowerShell, ...) the system handles quoting. If it still fails, put the whole command into `command`, or switch to `type: process`.

**Q: I want a task that is different per notebook.**

Put it under that notebook's `vx_notebook/tasks`; it only shows up in the menu while that notebook is the current one.



## Appendix: A Fairly Complete Configuration Example

```json
{
    "version": "0.1.3",
    "label": {
        "en_US": "Toolbox",
        "zh_CN": "工具箱"
    },
    "icon": "tools.svg",
    "type": "shell",
    "command": "echo",
    "args": ["${notebookName}", ""],
    "options": {
        "cwd": "${notebookFolder}"
    },
    "tasks": [
        {
            "label": "Open in Typora",
            "type": "process",
            "args": ["${buffer}"],
            "windows": { "command": "C:\\Program Files\\Typora\\Typora.exe" },
            "linux": { "command": "/usr/bin/typora" }
        },
        {
            "label": "Ask and echo",
            "command": "echo",
            "args": ["${input:who}", "${input:what}"],
            "inputs": [
                { "id": "who", "type": "pickString", "options": ["cat", "dove", "fish"], "default": "cat" },
                { "id": "what", "type": "promptString", "description": "Say something" }
            ]
        }
    ]
}
```

