# Review Before Apply

Use this checklist every time before modifying files.

## Required Steps
- Restate the user's request in one sentence.
- Inspect the relevant files first.
- Explain the likely root cause briefly.
- Provide a minimal unified diff for review.
- Wait for explicit user approval before applying the diff.

## Do Not Apply Yet If
- The user asked for suggestions only.
- The user asked for diff-only review.
- The repository state is unclear or there are conflicting local edits.
- The change would affect more files than necessary and needs scoping first.

## After Approval
- Apply only the reviewed diff unless the user approves additional changes.
- If implementation requires deviation from the reviewed diff, stop and present an updated diff.
- After applying, report what changed and what was not verified.

## Verification Rules
- Do not run build or tests unless approved or explicitly requested.
- If verification is skipped, say so plainly.
