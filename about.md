# Comment Manager

#### Say goodbye to daily comments!

Allows you to <cr>mass delete</c> comments from your levels based on a given criteria.

Also adds a global keybind (default is `control + k`) to view the queue of levels to delete comments from, or you can access from the button near the daily chest in the main menu.

## Features

### Basic:
- Disable Comments
    - All comments are deleted (unless whitelist is enabled)
- Use Whitelist
    - Never deletes comments from whitelisted users
- Use Wordlist
    - Only deletes comments that contain words from the wordlist
- Case Sensitive (only if wordlist enabled)
    - Deletes comments that match the given case (uppercase or lowercase)
- Delete Blocked
    - Delete any comments from blocked users

### Advanced:
- Persistent
    - Queued level never gets removed from queue unless manually deleted (persists through game restarts)
    - Saves in `saved.json`
- Use Regex
    - Delete any strings using the match string in the box provided
- Use Date Range
    - Specify a date range to delete comments between
    - Useful for daily levels
- Keep Logs
    - Logs deleted messages and who sent them
    - Default folder is the mod save directory

### Whitelist:
- Add and remove players per level
- Add friends or search for users by username or id
- Can add/remove whitelisted users as needed
- Can disable/enable whitelisted users as needed

## Wordlist:
- Comes with a default list of generally bad words
- Can add/remove words as needed
- Can disable/enable words as needed
