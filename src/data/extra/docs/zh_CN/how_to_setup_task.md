# VNote 任务（Task）配置指南

VNote 内置了一套简单的**任务**系统，其设计参照 [VSCode Tasks](https://code.visualstudio.com/docs/editor/tasks)，让你能在 VNote 内运行第三方程序和脚本。

用它来编译当前笔记、运行本地服务器、用其他编辑器打开文件、驱动 git，或任何你能编写脚本完成的事。

任务通过 **JSON 配置文件** 定义，运行结果输出到 VNote 的**控制台**面板。

> 本文依据当前代码（`src/task/`、`src/widgets/toolbarhelper.cpp`、`src/snippet/snippetmgr.cpp`）整理。与早期版本相比，加载目录、刷新方式、变量名、幻词列表等都有变化，请以本文为准。



## 快速上手

###  Hello World

1. 点击工具栏的 **任务** 菜单 → **添加任务** → **到 用户目录**，会打开用户任务目录：

   ```
   C:\Users\<用户名>\AppData\Local\VNote\VNote\tasks
   ```

   （该目录不存在时会自动创建；便携版则是 `<VNote 目录>\user_files\tasks`。）

2. 在目录里新建 `hello.json`：

   ```json
   {
       "command": "echo helloworld"
   }
   ```

3. 回到 VNote，点 **任务** → **重新加载**（改完配置**不需要重启**），菜单里就会多出 `hello` 这一项，点击即可运行。

运行任务时会自动激活下方的 **控制台** 面板，可以在其中看到输出。默认情况下，命令会交给系统默认的 shell 执行（Windows 为 `PowerShell.exe`，Linux/macOS 为 `/bin/bash`）。

###  菜单名称、图标与快捷键

任务在菜单里的显示名默认是**文件名**（根任务）或 `command`（子任务）。通过 `label`、`icon`、`shortcut` 可以自定义：

```json
{
    "label": "Hello",
    "icon": "tasks-solid.svg",
    "shortcut": "Alt+H, T",
    "command": "echo",
    "args": ["Hello tasks!"]
}
```

- `icon`：推荐 SVG（VNote 会按当前主题着色）。路径可以是**绝对路径**，也可以是**相对任务文件所在目录**的路径；文件不存在时该图标会被忽略。
- `shortcut`：格式形如 `Alt+H, T`，注册在菜单项上，按下即运行。
- `label` 中的 `&` 会被自动转义，不会被当作快捷键前缀，可放心使用。

### 子任务

任务可以任意层级嵌套，在菜单中显示为多级子菜单：

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

子任务会**继承父任务**的 `version`、`type`、`command`、`args`、`options.cwd`、`options.env`、`options.shell`；`label`、`icon`、`shortcut`、`inputs`、`tasks` 不会被继承（子任务没写 `label` 时显示为它的 `command`）。

> 提示：父任务若同时带有子任务，点击该菜单项只会展开子菜单；要运行父任务自身的命令，请给它配一个 `shortcut`（或者干脆不要在父任务上配命令）。

###  运行外部程序

把 `type` 设为 `process`，`command` 会被当作**可执行文件路径**直接启动（不经过 shell）：

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

其中 `${buffer}` 是当前编辑/查看文件的路径（见第 5 节）。

VNote 没有自己的控制台窗口，因此启动需要交互的命令行程序时，建议用 shell 另开一个终端窗口。例如 Windows 下 `start cmd "/c ..."`，Linux 下 `gnome-terminal --execute ...`：

```json
{
    "label": "Vim",
    "icon": "vim.svg",
    "type": "process",
    "command": "gnome-terminal",
    "args": ["--execute", "vim", "${buffer}"]
}
```

###  多平台与多语言

**多平台**：用 `windows`、`linux`、`osx` 三个键给出平台特定配置。平台配置会**覆盖**基础配置，但其中的 `tasks` 数组是**合并（追加）**的。

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

**多语言**：`label`、`command`、`args`（数组元素）、`options.env` 的值、`inputs` 的 `description`/`default`/`options` 等“可翻译”字段，除字符串外还可以传入 `{ "locale": "值" }` 形式的对象；找不到当前语言时取对象里的第一项。

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

当前语言取 VNote 的界面语言（设置里修改后需重启生效）。



## 任务的加载目录与菜单结构

###  三个层级

| 层级 | 目录 | 说明 |
| --- | --- | --- |
| 内置任务 | `<应用配置目录>/tasks` | 随程序分发，配置版本更新时自动复制。路径为 `%APPDATA%\VNote\VNote\tasks`；便携模式下为 `<VNote 目录>\vnotex_files\tasks`。当前内置的只有 **Git** 一组任务。 |
| 用户任务 | `<用户配置目录>/tasks` | 用户自己添加，目录不存在时会自动创建。路径为 `%LOCALAPPDATA%\VNote\VNote\tasks`；便携模式下为 `<VNote 目录>\user_files\tasks`。 |
| 笔记本任务 | `<笔记本根目录>/vx_notebook/tasks` | 只对**当前笔记本**生效，需要自己新建该目录。适合“只有这个笔记本才需要”的任务（例如发布某个站点）。 |

- 上述目录**及其子目录**下的所有 `*.json` 都会被识别为任务配置文件（每个文件对应菜单里的一组任务）。
- **加载时机**：启动时加载全部；切换笔记本时重新加载笔记本任务；修改配置后点 **任务 → 重新加载** 即可，**无需重启 VNote**。

###  菜单结构

```
任务 ▾
 ├─ 添加任务 ▸
 │   ├─ 到 用户目录       打开用户任务目录（不存在时自动创建）
 │   └─ 到 当前笔记本目录  打开 <笔记本>/vx_notebook/tasks（不存在时自动创建；没有当前笔记本时该项置灰）
 ├─ 重新加载
 ├─────────────
 │  （内置任务）
 ├─────────────
 │  用户任务              ← 只有“用户任务”和“笔记本任务”都存在时才会显示分组标题
 │  ...
 │  笔记本任务
 │  ...
```

即：**用户任务**与**笔记本任务**两组，在两者都有任务时会用标题标出来源目录，便于区分同名任务；只有一个来源时有任务时不加标题。



## 配置项参考

一个任务配置文件就是一个 JSON 对象（也可以理解为“一组任务”）。所有键均为可选。

| 键 | 类型 | 说明 |
| --- | --- | --- |
| `version` | 字符串 | 配置版本，默认 `0.1.3`。当前代码只接受 **0.x** 版本；写 `1.0.0` 及以上会被判定为“未知版本”而**忽略该任务**。一般不用写。 |
| `label` | 可翻译字符串 | 任务名。根任务默认取文件名，子任务默认取 `command`。 |
| `icon` | 字符串 | 图标路径，相对**任务文件所在目录**或绝对路径；需真实存在。 |
| `shortcut` | 字符串 | 快捷键，如 `Alt+H, T`。 |
| `type` | 字符串 | `shell`（默认）或 `process`。 |
| `command` | 可翻译字符串 | 任务命令。为空表示不执行（仅作为菜单分组用）。 |
| `args` | 可翻译字符串数组 | 命令参数。 |
| `options` | 对象 | 运行选项，见下。 |
| `tasks` | 数组 | 子任务，可无限嵌套。 |
| `inputs` | 数组 | 输入变量定义，见下。 |
| `messages` | 数组 | 预留，当前版本**已解析但未使用**。 |
| `windows`/`linux`/`osx` | 对象 | 平台特定配置，覆盖同名基础配置（其中 `tasks` 为追加合并）。 |

### `options`

| 键 | 说明 |
| --- | --- |
| `cwd` | 工作目录。缺省时依次尝试：**当前笔记本根目录** → **当前文件所在目录** → **任务配置文件所在目录**。 |
| `env` | 追加/覆盖的环境变量对象（值可翻译）。注意：**此处的值不做变量替换**。 |
| `shell.executable` | shell 可执行文件，仅当 `type` 为 `shell` 时读取。 |
| `shell.args` | shell 启动参数，缺省时按下面的默认表推导。 |

### `inputs`

| 键 | 说明 |
| --- | --- |
| `id` | 必填，供 `${input:id}` 引用。 |
| `type` | `promptString`（默认，弹输入框）或 `pickString`（弹选择框）。 |
| `description` | 提示文字（可翻译，支持变量）。 |
| `default` | 默认值（可翻译，支持变量）。`pickString` 时该值必须出现在 `options` 中。 |
| `password` | 布尔值，仅 `promptString` 生效，用于密码输入框。 |
| `options` | 字符串数组，仅 `pickString` 生效，为可选项列表（可翻译）。 |



## shell 与特殊字符处理

- **默认 shell**：Windows 为 `PowerShell.exe`，Linux/macOS 为 `/bin/bash`。
- **默认启动参数**：

  | Shell | 默认参数 |
  | --- | --- |
  | `cmd.exe` | `/C` |
  | `PowerShell.exe` | `-Command` |
  | `/bin/bash` | `-c` |

  其它 shell 默认不带参数，需要自己用 `options.shell.args` 指定。

- **命令拼接规则**（`src/task/shellexecution.cpp`）：
  - 当 shell 的**基名是 `bash`**（含默认的 `/bin/bash`）时：`command` 与各 `args` 中“含空格者”会被加上双引号，然后整体以空格拼成**一个字符串**，作为 `-c` 的单个参数传给 bash。
  - 其它 shell（Windows 下的 PowerShell、cmd 等）：`args` 作为**独立参数**逐个传给进程，由系统负责引号处理，不需要手动加引号。
  - 因此同一个任务在不同平台上可能需要不同的写法，这正是 `windows`/`linux` 平台配置存在的意义。

- **需要交互或常驻运行**的场合，请让 shell 另开终端窗口（见 1.4 的示例），VNote 本身不接管标准输入。



## 变量替换

变量语法为 `${变量名}`，可在 **`command`、`args`、`options.cwd`、`options.shell.args`** 中使用（另外 `inputs` 的 `description`/`default` 也支持变量）。未打开文件、没有笔记本等情况下，取不到值的变量会替换为**空字符串**。路径类变量的分隔符是**平台相关**的（Windows 为 `\`）。

###  预定义变量

假设：

- 当前笔记本名 `test-task`，根目录 `C:\notes\test-task`
- 当前打开的文件 `C:\notes\test-task\test2\note.md`
- 当前选中文本为 `a test`

则：

| 变量 | 含义 | 示例值 |
| --- | --- | --- |
| `${notebookFolder}` | 当前笔记本根目录 | `C:\notes\test-task` |
| `${notebookFolderName}` | 当前笔记本文件夹名 | `test-task` |
| `${notebookName}` | 当前笔记本名 | `test-task` |
| `${notebookDescription}` | 当前笔记本描述 | `This notebook for task test.` |
| `${buffer}` | 当前编辑/查看文件的路径 | `C:\notes\test-task\test2\note.md` |
| `${bufferNotebookFolder}` | 当前文件所属笔记本根目录 | `C:\notes\test-task` |
| `${bufferRelativePath}` | 当前文件相对笔记本根目录的路径 | `test2\note.md` |
| `${bufferName}` | 当前文件名 | `note.md` |
| `${bufferBaseName}` | 当前文件名（不含扩展名） | `note` |
| `${bufferDir}` | 当前文件所在目录 | `C:\notes\test-task\test2` |
| `${bufferExt}` | 当前文件扩展名（不含点） | `md` |
| `${selectedText}` | 当前选中文本 | `a test` |
| `${lineNumber}` | 当前光标所在行号（阅读模式下为顶部可见行），从 1 开始；未打开文件或该窗口没有行号概念时为空字符串 | `2` |
| `${cwd}` | 本次任务的工作目录（见 `options.cwd` 规则） | `C:\notes\test-task` |
| `${taskFile}` | 当前任务的配置文件路径 | |
| `${taskDir}` | 当前任务配置文件所在目录 | |
| `${exeFile}` | VNote 可执行文件路径 | `C:\Programs\vnote3\vnote.exe` |
| `${pathSeparator}` | 当前平台路径分隔符 | `\` |
| `${notebookTaskFolder}` | 当前笔记本任务目录 | `C:\notes\test-task\vx_notebook\tasks` |
| `${userTaskFolder}` | 用户任务目录 | `C:\Users\me\AppData\Local\VNote\VNote\tasks` |
| `${appTaskFolder}` | 内置（应用）任务目录 | `C:\Users\me\AppData\Roaming\VNote\VNote\tasks` |
| `${userThemeFolder}` / `${appThemeFolder}` | 用户/内置主题目录 | |
| `${userDocsFolder}` / `${appDocsFolder}` | 用户/内置文档目录 | |

> 早期版本的 `${file}`、`${fileBasename}` 等名字已改为 `${buffer*}` 系列；`${execPath}` 现为 `${exeFile}`，`${taskDirname}` 现为 `${taskDir}`，`${notebookFolderBasename}` 现为 `${notebookFolderName}`。

### 幻词（magic）

`${magic:名称}` 实际是复用 VNote 的**片段（Snippet）**机制：`${magic:date}` 会被还原成片段符号 `%date%` 并展开。

内置片段名（见 `SnippetMgr::loadBuiltInSnippets`）：

| 分类 | 名称 |
| --- | --- |
| 日/周/月/年 | `d`、`dd`、`ddd`、`dddd`、`M`、`MM`、`MMM`、`MMMM`、`yy`、`yyyy`、`w`、`ww` |
| 时/分/秒 | `H`、`HH`、`m`、`mm`、`s`、`ss` |
| 组合 | `date`（`2021-02-24`）、`da`（`20210224`）、`time`（`16:51:02`）、`datetime`（`2021-02-24_16:51:02`） |
| 当前笔记 | `note`（笔记文件名）、`no`（不含扩展名的笔记名） |

例如 `${magic:datetime}` → `2021-02-24_16:51:02`。

**可以自定义幻词**：在 `<用户配置目录>/snippets/` 下放置自己的片段 JSON（与编辑器里 `%name%` 片段是同一套），就能用 `${magic:你的名字}` 引用；与内置同名会被跳过。

### 环境变量

`${env:名称}` 取系统环境变量，例如：

- `${env:ComSpec}` → `C:\Windows\system32\cmd.exe`
- `${env:TEMP}` → `C:\Users\me\AppData\Local\Temp`

###  配置变量

`${config:路径}` 读取 VNote 配置文件（`vnotex.json`）中的值，例如 `${config:core.locale}`、`${config:metadata.version}`。

只支持 `对象.键` 与 `数组[下标]` 的写法，且只返回字符串/数字/布尔（布尔转为 `1`/`0`，数组与对象返回空字符串）。

### 输入变量

`${input:id}` 引用 `inputs` 里定义的一项，运行时弹窗询问：

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

`pickString` 示例：

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

同一个字段里重复出现同一输入变量时只会询问一次；取消输入会**中止整个任务**。

###  Shell 变量

`${shell:命令}` 会用默认 shell 执行该命令并把**标准输出**作为变量值，例如：

- `${shell:git rev-parse --abbrev-ref HEAD}` → `main`
- `${shell:whoami}` → `me`

工作目录与任务一致，超过 1 秒未完成会中止任务执行。

###  求值规则

- 变量可嵌套出现（替换后的文本里若仍含变量会继续求值），但不支持 `${a${b}}` 这种**变量名本身嵌套**。
- 变量值里的路径使用平台相关分隔符。
- `${magic:*}` 与 `${shell:*}` 的值若包含注释/换行，请自行注意 shell 语法。



## 输出与错误

- 任务输出（stdout/stderr）汇总到 **控制台** 停靠面板，运行任务时自动激活；面板会依次尝试 `UTF-8` → `System` → `UTF-16` → `GB18030` 解码，全部失败时退回本地 8 位编码。
- 运行时会在输出中插入三行提示：

```
[Task (Hello) started]
...任务的输出...
[Task (Hello) finished (0)]
```

出错时输出：

```
[Task (Hello) error occurred (0)]
```

括号中的数字是 Qt 进程错误码：

| 错误码 |                    含义                     |
| ------ | ------------------------------------------ |
| 0      | 启动失败（命令或参数有误、可执行文件不存在等） |
| 1      | 进程崩溃                                    |
| 2      | 超时                                        |
| 3      | 写入错误                                    |
| 4      | 读取错误                                    |
| 5      | 未知错误                                    |

- 当前版本**不支持**在控制台里向任务进程输入内容（代码中仍是 TODO）。需要交互的程序请按 1.4 的方式另开终端窗口。



## 实战示例

###  Git（内置任务）

**任务 → 重新加载** 后即可在菜单底部看到内置的 `Git` 组，包含：

| 菜单项 | 等价命令 |
| --- | --- |
| 初始化 | `git init -b main` |
| 状态 | `git status` |
| 提交 | `git add -A -- . && git commit --message="${input:msg}"`（Windows 平台用 `; if ($?) { ... }`） |
| 上传 | `git push` |
| 下载 | `git pull --no-rebase` |
| 日志 | `git log -10 --graph --pretty=format:'%h -%d %s (%cr) <%an>' --abbrev-commit` |

- 「提交」会弹窗询问提交信息，默认值是 `更新笔记于 ${magic:datetime}`。
- 工作目录默认是**当前笔记本根目录**（`options.cwd` 缺省规则），所以请先切换到目标笔记本再运行。
- 提示：Git 默认会把非 ASCII 文件名显示成十六进制，可执行 `git config --global core.quotepath false` 关闭。

###  只对某个笔记本生效的任务（笔记本任务）

在 `<笔记本>/vx_notebook/tasks` 下放 `publish.json`，例如发布站点：

```json
{
    "label": "Publish",
    "command": "hexo clean && hexo generate && hexo deploy",
    "options": { "cwd": "${shell:git rev-parse --show-toplevel}" }
}
```

（该目录可用 **任务 → 添加任务 → 到 当前笔记本目录** 快速打开。）

### 交互式程序：编译并另开窗口运行

```json
{
    "command": "g++ \"${buffer}\" -o \"${bufferBaseName}\"; if ($?) { start cmd \"/c `\"${bufferBaseName}`\" & pause\" }"
}
```

注意：这类长命令**不要**拆成 `command` + `args`，不同 shell 的引号规则差异很大（见第 4 节）。

### 后台服务：在笔记本目录起 HTTP 服务器

```json
{
    "command": "start cmd.exe \"/c python -m http.server\" ; start http://localhost:8000"
}
```

## 尚未实现 / 与 VSCode Tasks 的差异

以下配置项在 VSCode 中存在，但当前 VNote 版本**尚未实现**（写了也不会生效）：

- 任务依赖：`dependsOn`、`dependsOrder`
- 输出控制：`presentation`（`reveal`/`clear` 等）
- 自动触发：`runOptions.runOn`（打开/关闭笔记本、启动/退出 VNote 时自动运行）
- 任务 `id`、`group`
- `messages` 配置：已能解析，但当前没有用到
- 从“通用入口/命令面板”触发任务（目前只能通过菜单或快捷键）
- 任务配置的图形化编辑器（只能手写 JSON）
- 控制台内的进程交互输入

除此之外，`type`（`shell`/`process`）、`command`/`args`、变量、输入变量、平台/语言覆盖等都与 VSCode 的写法基本兼容。



## 常见问题（FAQ）

**Q：写了 JSON，菜单里却没有出现？**

依次检查：

1. 文件是否放在三个任务目录之一（或它们的子目录）下，且扩展名是 `.json`；
2. 是否点了 **任务 → 重新加载**；
3. 若放在 `<笔记本>/vx_notebook/tasks`，需要先**打开/切换到该笔记本**；
4. `version` 若写成 `1.0.0` 及以上，任务会被忽略（日志中会提示 unknown task version）；
5. JSON 语法错误、`icon` 指向不存在的文件等会在日志（`<用户配置目录>/vnotex.log`）中留下 warning。

**Q：为什么子任务明明没写 `command` 却能运行？**

子任务会继承父任务的 `type`/`command`/`args`/`options`（见 1.3）。

**Q：为什么点击带子任务的菜单项没反应？**

它会展开子菜单；子任务/父任务自身的命令请用各自菜单项或快捷键触发。

**Q：输出的中文是乱码？**

先在控制台确认输出编码；VNote 会按 `UTF-8 → System → UTF-16 → GB18030` 尝试，通常不需要干预。若某个工具输出的是 GBK，仍会按顺序命中。

**Q：`${buffer}` 为空？**

当前没有打开任何文件时要取不到值；同理，没有打开笔记本时 `${notebookFolder}` 系列为空。

**Q：路径里带空格？**

`type: shell` 且 shell 为 bash 时 VNote 会自动为含空格的参数加双引号；其它 shell（PowerShell 等）由系统处理引号。若仍出错，把整条命令写进 `command`，或改用 `type: process`。

**Q：想要“每个笔记本不同”的任务？**

放到该笔记本的 `vx_notebook/tasks` 下即可，只有切换到该笔记本时才会出现在菜单里。



## 附录：一个较完整的配置示例

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
