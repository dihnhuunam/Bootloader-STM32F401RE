# AGENTS.md

## Purpose
This folder defines the working rules between the user and Codex for this repository.
Codex should read this file first before making suggestions or changes.

## Default Workflow
- Do not edit any file before the user reviews the proposed diff.
- Do not run `apply_patch` or any other file-modifying action until the user explicitly approves.
- Default response mode for code changes is: root cause, affected files, then diff only.
- Keep suggestions minimal and scoped to the user's request.

## Approval Rules
- The default approval phrase is any clear confirmation from the user such as `approve`, `ok sửa`, `apply`, or equivalent.
- If approval is not explicit, stay in review mode and only provide analysis, plan, or diff.
- If a previous turn included unapproved edits, stop and ask whether to revert or continue from the current state.

## Build And Verification
- Do not run a full build unless the user explicitly asks for it.
- Prefer targeted verification steps over full rebuilds when possible.
- If verification requires a command that may take time, say what will be checked before running it.

## Communication Style
- Use Vietnamese by default unless the user asks otherwise.
- Keep answers concise and technical.
- When proposing a fix, prefer a unified diff block over long prose.
- Mention assumptions clearly if repository state is ambiguous.

## Starter Prompt For New Sessions
The user can start a session with:
- `Read .codex/AGENTS.md and follow it.`
- `Review .codex workflow files before doing anything.`
