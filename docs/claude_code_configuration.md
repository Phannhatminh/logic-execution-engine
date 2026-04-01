# Claude Code — Configuration Guide

A reference for all Claude Code configuration files, their locations, scoping, how to manage them, and what each feature does.

---

## Four Persistence Systems

Claude Code has four complementary persistence systems:

1. **CLAUDE.md** — Instructions and guidance for Claude's behavior. Markdown files.
2. **settings.json** — Configuration: permissions, hooks, environment variables, model selection, plugins. JSON files.
3. **Auto Memory** — Machine-local learning that persists across sessions. Markdown files.
4. **Keybindings** — Keyboard shortcuts. A single JSON file.

---

## 1. CLAUDE.md — Instructions

CLAUDE.md files contain human-written instructions that Claude follows. They tell Claude how to behave in a project — coding standards, architecture decisions, build commands, preferences.

### Locations

**Organization-wide (managed):**
`/Library/Application Support/ClaudeCode/CLAUDE.md` on macOS. Deployed by admins. Applies to all users in the organization. Cannot be overridden by user or project settings. Used for company-wide policies, security rules, compliance requirements.

**User-wide:**
`~/.claude/CLAUDE.md`. Your personal instructions that apply to every project. Not shared via git. Use this for your personal coding style, preferred patterns, tooling shortcuts — anything that's about you, not about a specific project.

**Project-level:**
`./CLAUDE.md` or `./.claude/CLAUDE.md` at the repository root. Shared via git. The whole team sees these. Use this for project architecture, build/test commands, coding standards, naming conventions.

### How They Load

Claude walks up the directory tree from your working directory and loads all CLAUDE.md files it finds. More specific (deeper) locations override broader ones. Subdirectory CLAUDE.md files load lazily — only when Claude reads files in those directories. Keep each file under 200 lines to conserve context.

You can use `@path/to/file` syntax inside CLAUDE.md to include additional files.

### Modular Rules

For path-specific instructions that shouldn't consume context unless needed, use `.claude/rules/`. Create markdown files like `.claude/rules/code-style.md` or `.claude/rules/testing.md`. These load only when Claude reads files in matching paths, saving context space.

---

## 2. settings.json — Configuration

Settings files control Claude Code's behavior: what tools are allowed, what happens before and after tool use, environment variables, model selection, and more.

### Four-Layer Hierarchy

Settings cascade from most specific to broadest. Higher priority wins:

**Priority 1 — Managed (highest):**
System directories, deployed by admins. Cannot be overridden. Used for enterprise security policies, compliance enforcement, blocked tools or file paths.

**Priority 2 — Local:**
`.claude/settings.local.json` in the project. Auto-gitignored. Your personal overrides for this project on this machine. Use for machine-specific secrets, testing new configs before sharing, personal overrides that only work on your setup.

**Priority 3 — Project:**
`.claude/settings.json` in the project. Committed to git, shared with the team. Use for team-wide permissions, shared hooks, project MCP servers, project environment variables.

**Priority 4 — User (lowest):**
`~/.claude/settings.json` in your home directory. Applies to all projects. Use for your preferred model, personal theme, personal plugins, personal MCP servers.

### What You Can Configure

**Permissions** control which tools Claude can use without asking.

The `permissions` field has `allow` and `deny` arrays. Each entry is a tool name with an optional argument pattern. Claude can use allowed tools without prompting. Denied tools are blocked entirely.

Example: `"allow": ["Bash(npm run *)"]` lets Claude run any `npm run` command without asking. `"deny": ["Read(./.env*)"]` blocks Claude from reading any .env files.

Permissions are evaluated with deny taking precedence over allow. If something matches both, it's denied.

**Hooks** are shell commands or HTTP calls that execute automatically at specific lifecycle events.

Hook events include:
- `SessionStart` — when a session begins. Use for setup, loading context.
- `UserPromptSubmit` — when you submit a prompt. Use for input validation, logging.
- `PreToolUse` — before a tool executes. Use for validation, safety checks. Can block the tool.
- `PostToolUse` — after a tool executes. Use for formatting, linting, notifications.
- `PreCompact` / `PostCompact` — before and after context compaction. Use for preserving important context.
- `Stop` — when the session ends. Use for cleanup, saving state.

Each hook has a `matcher` (which tool or event to match) and a list of hook actions. Actions can be `command` (run a shell script), `http` (POST to a URL), `prompt` (ask Claude to validate), or `agent` (delegate to a subagent).

Hooks configured in managed settings cannot be disabled. Project hooks apply to the whole team. User hooks apply to all your projects. Use `"disableAllHooks": true` to temporarily turn off all non-managed hooks.

**Environment variables** set via the `env` field are available in all sessions. Useful for setting `DEBUG=1`, API keys, or project-specific variables that tools need.

**Model selection** via the `model` field sets the default model. Can be overridden per-session with CLI args or the `/model` command.

**Auto memory** can be enabled or disabled with `autoMemoryEnabled`. When enabled, Claude saves notes about your corrections and patterns for future sessions. When disabled, no memory files are created or read.

**MCP servers** (Model Context Protocol) extend Claude's capabilities by connecting to external tools and services. Configured in `.mcp.json` at the project root (shared) or `~/.claude.json` (personal). Each entry specifies a command to run and environment variables it needs. MCP servers can provide custom tools, data sources, or integrations.

**Plugins** extend Claude Code with additional functionality. Enabled via `enabledPlugins` in settings. Plugins can provide new tools, hooks, or behaviors.

### Example settings.json

```json
{
  "permissions": {
    "allow": ["Bash(npm run *)", "Bash(make *)"],
    "deny": ["Bash(rm -rf *)", "Read(./.env*)"]
  },
  "hooks": {
    "PostToolUse": [
      {
        "matcher": "Write",
        "hooks": [
          {
            "type": "command",
            "command": "npx prettier --write $CLAUDE_FILE_PATH",
            "timeout": 10
          }
        ]
      }
    ]
  },
  "env": {
    "DEBUG": "1"
  },
  "model": "claude-opus-4-6",
  "autoMemoryEnabled": true
}
```

---

## 3. Auto Memory — Machine-Local Learning

Auto memory lets Claude remember things across sessions — your corrections, project patterns, debugging notes, user preferences. It's stored as plain markdown files on your machine.

### Where It Lives

```
~/.claude/projects/<project-path-hash>/memory/
├── MEMORY.md              — Index, first 200 lines loaded every session
├── topic_file_1.md        — Loaded on demand when relevant
├── topic_file_2.md
└── ...
```

The `<project-path-hash>` is derived from the git repository path. All directories within the same repo share one memory directory.

### How It Works

**MEMORY.md** is the index. The first 200 lines are loaded at every session start. It should contain only brief pointers to topic files, not full content. Lines beyond 200 are truncated and never seen.

**Topic files** (like `project_goals.md`, `feedback_testing.md`) are NOT loaded at startup. Claude reads them on demand when the topic seems relevant. Each topic file has frontmatter with a name, description, and type that help Claude decide when to read it.

Claude creates and updates memory files based on:
- Explicit requests ("remember that I prefer X")
- Corrections ("no, don't do it that way")
- Patterns it notices across the conversation

All files are plain markdown. You can edit or delete them manually at any time. Browse with the `/memory` command.

### Memory Types

**User memories** — information about you: your role, expertise, preferences. Help Claude tailor its behavior to you.

**Feedback memories** — corrections and confirmations: what to avoid, what worked well. Help Claude not repeat mistakes and not drift from validated approaches.

**Project memories** — information about ongoing work: goals, deadlines, decisions, who's doing what. Help Claude understand context behind requests.

**Reference memories** — pointers to external resources: where bugs are tracked, which dashboard to check, where docs live. Help Claude know where to look.

### Scoping Limitations

Memory is **machine-local only**. Not synced across machines, not backed up to cloud.

Memory is **per git repo**. Each repository gets its own memory directory. But if multiple projects share a parent git root or path hash, they share a memory directory. In that case, label each memory file with which project it applies to, so future sessions know what's relevant and what isn't.

You can set a custom memory directory with `"autoMemoryDirectory": "~/my-custom-dir"` in settings.

### Effective Management

Keep MEMORY.md concise — it's an index, not a dump. Use separate topic files for separate concerns. Delete stale memories that no longer apply. Review periodically with `/memory`. If memory is shared across projects, prefix or label files clearly.

---

## 4. Keybindings — `~/.claude/keybindings.json`

A single file at `~/.claude/keybindings.json`. Global, user-level only. Controls keyboard shortcuts in the Claude Code CLI.

### How It Works

```json
{
  "bindings": [
    {
      "context": "Chat",
      "bindings": {
        "ctrl+e": "chat:externalEditor",
        "ctrl+u": null
      }
    }
  ]
}
```

**Contexts** specify where the binding applies: Chat, Global, Autocomplete, Confirmation, Tabs, Help, Transcript.

**Key syntax:** `ctrl+k`, `shift+tab`, `ctrl+k ctrl+s` (chords — press first combo, release, press second). Uppercase letters imply Shift.

**Unbind** a default by setting it to `null`.

**Reserved keys:** Ctrl+C and Ctrl+D cannot be rebound.

Changes are auto-detected — no restart needed.

---

## 5. MCP Server Configuration

MCP (Model Context Protocol) servers extend Claude with custom tools and data sources. They run as external processes that Claude communicates with.

### Project MCP Servers: `.mcp.json`

At the project root. Committed to git, shared with the team. Defines MCP servers that everyone on the project needs — shared databases, internal APIs, team tools.

```json
{
  "mcpServers": {
    "github": {
      "command": "npx",
      "args": ["@modelcontextprotocol/server-github"],
      "env": {"GITHUB_PERSONAL_ACCESS_TOKEN": "..."}
    }
  }
}
```

### User MCP Servers: `~/.claude.json`

In your home directory. Personal MCP servers that apply across all projects — your personal tools, integrations, services.

### Managed MCP: `managed-mcp.json`

System directories (same as managed settings). Admin-deployed. Enforces MCP server allowlists/denylists organization-wide.

---

## 6. Directory Structures

### Project `.claude/` Directory

```
.claude/
├── CLAUDE.md                  — Project instructions (shared)
├── rules/                     — Path-specific rules (shared)
│   └── *.md
├── settings.json              — Project settings (shared, committed)
├── settings.local.json        — Local overrides (gitignored, personal)
├── agents/                    — Custom subagents (shared)
│   └── *.md
└── hooks/                     — Hook scripts (shared)
    └── *.sh
```

### User `~/.claude/` Directory

```
~/.claude/
├── CLAUDE.md                  — User instructions (all projects)
├── settings.json              — User settings (all projects)
├── keybindings.json           — Keyboard shortcuts
├── rules/                     — User-level rules
│   └── *.md
├── agents/                    — User subagents
│   └── *.md
└── projects/                  — Auto memory (per-project, machine-local)
    └── <project-hash>/memory/
        ├── MEMORY.md
        └── *.md
```

---

## 7. Precedence Summary

**Settings** — highest priority wins:
Managed → CLI arguments → Local → Project → User

**CLAUDE.md** — most specific wins:
Managed policy → Deepest directory → Project root → User

**Memory** — no precedence. Single directory per project, machine-local only.

---

## 8. Quick Reference

**Set project coding standards** — edit `./CLAUDE.md` or `./.claude/CLAUDE.md`

**Set my personal preferences** — edit `~/.claude/CLAUDE.md`

**Allow a bash command for the team** — add to `.claude/settings.json` under `permissions.allow`

**Allow a command just for me** — add to `.claude/settings.local.json` under `permissions.allow`

**Add a hook for the team** — add to `.claude/settings.json` under `hooks`

**Set an environment variable** — add to `.claude/settings.json` under `env`

**Change default model** — set `model` in `~/.claude/settings.json`

**Customize keyboard shortcuts** — edit `~/.claude/keybindings.json`

**Add a team MCP server** — edit `.mcp.json` at project root

**Add a personal MCP server** — edit `~/.claude.json`

**Check/edit stored memories** — run `/memory` or edit files in `~/.claude/projects/<project>/memory/`

**Add path-specific instructions** — create `.claude/rules/topic.md`
