#!/usr/bin/env python3
import sys

def generate_comment_line(text):
    """Generate a centered 80-char comment line with proper spacing"""
    max_width = 80
    comment_start = "/* "
    comment_end = " */"
    padding_char = '='

    # Add single spaces around text
    padded_text = f" {text.strip()} " if text else "  "
    available_width = max_width - len(comment_start) - len(comment_end)

    if len(padded_text) > available_width:
        print(f"❌ Error: Text must be ≤{available_width-2} characters (yours was {len(text)})", file=sys.stderr)
        return None

    total_padding = available_width - len(padded_text)
    left_padding = total_padding // 2
    right_padding = total_padding - left_padding

    return (comment_start +
            padding_char * left_padding +
            padded_text +
            padding_char * right_padding +
            comment_end)

def copy_to_clipboard(text):
    """Try copying to clipboard with friendly error handling"""
    try:
        if sys.platform == 'darwin':
            import subprocess
            subprocess.run('pbcopy', universal_newlines=True, input=text)
            return True
        elif sys.platform == 'win32':
            import win32clipboard
            win32clipboard.OpenClipboard()
            win32clipboard.EmptyClipboard()
            win32clipboard.SetClipboardText(text)
            win32clipboard.CloseClipboard()
            return True
        else:
            import subprocess
            subprocess.run(['xclip', '-selection', 'c'], input=text.encode('utf-8'))
            return True
    except Exception as e:
        print(f"⚠️ Couldn't copy to clipboard: {str(e)}", file=sys.stderr)
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"✨ Usage: {sys.argv[0]} \"Your comment text here\"", file=sys.stderr)
        print("💡 Pro tip: Use quotes for multi-word comments", file=sys.stderr)
        sys.exit(1)

    text = " ".join(sys.argv[1:])
    comment_line = generate_comment_line(text)

    if comment_line:
        if copy_to_clipboard(comment_line):
            print(f"✅ Success! Copied to clipboard:\n{comment_line}")
        else:
            print(f"📋 Here's your comment (paste manually):\n{comment_line}")
