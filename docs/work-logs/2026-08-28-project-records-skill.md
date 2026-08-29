# 工作记录：把三类文档规则抽成可复用的 skill

- **日期：** 2026-08-28
- **分支：** `fix/download-review-findings`（PR #39，基于 `main` 的 `c894252`）
- **关联文档：** [同日工作记录](2026-08-28-download-review-findings.md)、
  [PR 记录](../pull-requests/2026-08-28-download-review-findings.md)

## 一、用户的请求

> "You can extract the organization rules for these three types of documents and
> implement them as a 'skill' to facilitate future extensibility and reuse; this skill
> can be placed in the `agent/skill` directory, with a reference to `./agent/skill`
> added to `CLAUDE.md`."

把上一条请求里刚写进 `CLAUDE.md` 的三条记录规则（工作记录 / 审查归档 / PR 记录）抽出来，
做成一份独立的、可扩展可复用的 skill 文档，放到 `agents/` 下，并让 `CLAUDE.md` 指向它。

## 二、制定的计划

1. 先看仓库里已有的 `agents/` 布局（已有 `agents/prompts/*.md`），据此把 skill 放在
   `agents/skills/project-records/SKILL.md`，与既有目录风格一致。
2. skill 内容不只是搬运规则，还要补上 `CLAUDE.md` 里放不下的部分：三类文档的模板骨架、
   互相链接的方式、以及「如何新增第四类记录」的扩展说明。
3. 用带 YAML frontmatter 的 `SKILL.md` 格式书写，这样将来若要让 Claude Code 自动发现，
   整个目录可以原样搬到 `.claude/skills/` 下，不必改写内容。
4. `CLAUDE.md` 里保留「什么时候要写、写到哪、用什么语言」的触发表（这部分必须不加载
   skill 也能看到），细节改为指向 skill。
5. 按新规则给本次请求补一份工作记录，也就是本文件。

写到第 1 步时发现 `.gitignore` 第 111 行整个忽略了 `agents/`（注释写着 `# Agents`，
`agents/prompts/*.md` 本来就是不入库的本地文件）。放在那里的 skill 永远提交不上去，
别人克隆下来只会看到 `CLAUDE.md` 指向一个不存在的文件，与「可复用」的目标正好相反。
因此追加一步：把 `agents/` 改成 `agents/*` 并加一条 `!agents/skills/` 反向规则，
只把共享的 skill 目录放出来，`agents/prompts/` 维持忽略不变。

## 三、具体改了哪些文件

| 文件 | 改动 |
|------|------|
| `agents/skills/project-records/SKILL.md` | 新增。三类记录的触发条件、目录、语言对照表；三类文档各自的模板骨架；跨文档互链要求；「没跑过的命令不许打勾」等通用规则；末尾的扩展指引 |
| `CLAUDE.md` | `## Mandatory records` 一节由三条散文改为触发表 + 指向 skill 的链接，并保留验证诚实性与 AI 披露两条硬规则 |
| `.gitignore` | `agents/` 改为 `agents/*` 并加 `!agents/skills/`，让 skill 能入库；`agents/prompts/` 仍然被忽略 |
| `docs/work-logs/2026-08-28-project-records-skill.md` | 本文件（新增） |

## 四、验证情况

纯文档改动，没有代码变化，因此没有编译也没有跑测试；`build.cmd` 仍然只能在 Windows 上跑。
已确认的有三点：`CLAUDE.md` 中的相对链接 `agents/skills/project-records/SKILL.md` 指向的
文件确实存在；skill 里引用的 `docs/` 子目录与 `.github/pull_request_template.md` 也都存在；
`git status --untracked-files=all` 确认改完 `.gitignore` 后待入库的新文件只有
`agents/skills/project-records/SKILL.md` 与本文件，`agents/prompts/` 没有被顺带放出来。

## 五、遗留事项

- **skill 目前不会被 Claude Code 自动加载。** Claude Code 只在 `.claude/skills/<name>/SKILL.md`
  下发现 skill；放在 `agents/skills/` 里它是一份被 `CLAUDE.md` 引用的文档，靠 `CLAUDE.md`
  的指引被读到，而不是一个可以用 `/project-records` 调用的技能。若希望它可被直接调用，
  把 `agents/skills/project-records/` 整个目录移到 `.claude/skills/` 即可，文件内容不用改。
- 本次改动尚未提交；提交后会并入已开启的 PR #39，届时需要同步更新该 PR 的正文说明。
