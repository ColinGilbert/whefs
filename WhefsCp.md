whefs-cp is a tool similar to the conventional <tt>cp</tt> command. It can import local files into an EFS and export them from an EFS.

Usage: whefs-cp `[flags]` EFS\_file file1 `[...fileN]`

Aside from the [common command flags](WhefsTools.md), whefs-cp supports these flags:

  * -i = Import local file into the EFS. This is the default.
  * -x = Export EFS pseudofile to a local file.

This tool has some notable limitations, some of which are:

  * When extracting files with path separators in the names, the target path(s) must already exist.
  * When used with -x, the file names may not currently be wildcards.
  * It has no mechanism for renaming a file during the copy process nor setting a destination path.
  * The EFS doesn't support platform-specific access rights, so access rights are not carried over. This might be changed someday.