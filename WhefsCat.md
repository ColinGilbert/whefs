whefs-cat is a tool similar to the conventional <tt>cat</tt> command. It exports files from inside an EFS to standard output.

Usage: `whefs-cat [flags] VFS_file file1 [...fileN]`

The filename arguments must be full names of files in the EFS (wildcards are not currently supported).

The output goes to standard out (normally your console).

This tool has no command-line arguments other than the [common whefs tool command flags](WhefsTools.md).