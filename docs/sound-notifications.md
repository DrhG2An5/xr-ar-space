# Sound Notifications in Claude Code (Windows 10)

Claude Code has **no built-in audio** — all sound notifications are configured via the **hooks system**.

## Quick Setup (Copy to `~/.claude/settings.json`)

```json
{
  "hooks": {
    "Notification": [
      {
        "matcher": "idle_prompt",
        "hooks": [{ "type": "command", "command": "powershell.exe -Command \"[Console]::Beep(1000, 300)\"" }]
      },
      {
        "matcher": "permission_prompt",
        "hooks": [{ "type": "command", "command": "powershell.exe -Command \"[Console]::Beep(900, 150); Start-Sleep -Milliseconds 100; [Console]::Beep(900, 150)\"" }]
      }
    ],
    "PostToolUseFailure": [
      {
        "matcher": "",
        "hooks": [{ "type": "command", "command": "powershell.exe -Command \"[Console]::Beep(300, 500)\"" }]
      }
    ]
  }
}
```

## Key Hook Events for Notifications

| Event | When | Best For |
|-------|------|----------|
| `Notification` (matcher: `idle_prompt`) | Claude finished, waiting for input | "I'm done" alert |
| `Notification` (matcher: `permission_prompt`) | Permission dialog appeared | Urgent — user action needed |
| `PostToolUseFailure` | A tool failed | Error alert |
| `Stop` | Claude finished response | Task completion |
| `StopFailure` | API error (rate limit, auth) | Critical failure |
| `PermissionRequest` | Tool needs approval | User action needed |

## Sound Options on Windows

### Option A: Console Beep (zero dependencies)
```powershell
[Console]::Beep(1000, 300)  # frequency Hz, duration ms
```

### Option B: System WAV files
```powershell
$s = New-Object System.Media.SoundPlayer 'C:\Windows\Media\Notify.wav'; $s.PlaySync()
```
Available: `Alarm01.wav`-`Alarm05.wav`, `Notify.wav`, `tada.wav`, `Windows Error.wav`

### Option C: Custom WAV
```powershell
$s = New-Object System.Media.SoundPlayer 'C:\Users\NG-dev\Sounds\custom.wav'; $s.PlaySync()
```

## Tone Patterns

| Purpose | Command |
|---------|---------|
| Info/idle | `[Console]::Beep(1000, 300)` |
| Error | `[Console]::Beep(300, 500)` |
| Success | `[Console]::Beep(1200, 200); Start-Sleep -Milliseconds 100; [Console]::Beep(1200, 200)` |
| Urgent/permission | `[Console]::Beep(900, 150); Start-Sleep -Milliseconds 100; [Console]::Beep(900, 150)` |
| Error descending | `[Console]::Beep(600, 200); Start-Sleep -Milliseconds 100; [Console]::Beep(400, 300)` |

## Frequency Guide
- 300-500 Hz = errors (low, ominous)
- 800-1000 Hz = info/waiting (standard)
- 1200+ Hz = success (high, bright)

## Config Locations
- **Global**: `~/.claude/settings.json` (all projects)
- **Project**: `.claude/settings.json` (this project only)
- **Local**: `.claude/settings.local.json` (gitignored, personal)

## Troubleshooting
- If hook doesn't fire: check `/hooks` in Claude Code, verify JSON syntax
- If no sound: check Windows volume, try `powershell.exe -ExecutionPolicy Bypass -Command ...`
- JSON parse errors: ensure shell profile doesn't output text in non-interactive mode

## Docs
- Hooks guide: https://code.claude.com/docs/en/hooks-guide.md
- Settings: https://code.claude.com/docs/en/settings.md
