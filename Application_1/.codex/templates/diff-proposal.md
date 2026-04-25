# Diff Proposal Template

Use this template when the user asks for a fix but has not yet approved file changes.

## Summary
- Request:
- Root cause:
- Affected files:

## Proposed Diff
```diff
diff --git a/<file> b/<file>
--- a/<file>
+++ b/<file>
@@
- old line
+ new line
```

## Notes
- Keep the diff minimal.
- Do not apply until the user explicitly approves.
- If there are multiple valid approaches, present the safest one first.
