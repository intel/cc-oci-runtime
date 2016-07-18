# Contributing to clr-oci-runtime

clr-oci-runtime is an open source project licensed under the [GPL v2 License] (https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).

## Coding Style

The coding style for clr-oci-runtime is roughly K&R with function names in
column 0, and variable names aligned in declarations.

The right results can be almost achieved by doing the following.

* GNU Emacs: if you're not using auto-newline, the following should do the right thing:

```
	(defun clr-oci-runtime-c-mode-common-hook ()
	  (c-set-style "k&r")
	  (setq indent-tabs-mode t
	        c-basic-offset 8))
```

* VIM: the default works except for the case labels in switch statements.  Set the following option to fix that:

```
	setlocal cinoptions=:0
```

* Indent: can be used to reformat code in a different style:

```
	indent -kr -i8 -psl
```

## Certificate of Origin

In order to get a clear contribution chain of trust we use the [signed-off-by language] (https://01.org/community/signed-process)
used by the Linux kernel project.

## Patch format

Beside the signed-off-by footer, we expect each patch to comply with the following format:

```
       Change summary

       More detailed explanation of your changes: Why and how.
       Wrap it to 72 characters.
       See [here] (http://chris.beams.io/posts/git-commit/)
       for some more good advices.

       Signed-off-by: <contributor@foo.com>
```

For example:

```
	Annotations spec handler test fix.
    
	json-glib is clever enough to know that an object cannot
	have a null key, but seemingly has a bug where it doesn't
	release some memory in that scenario (which causes the
	runtimes valgrind test to fail).
    
	Signed-off-by: James Hunt <james.o.hunt@intel.com>
```

## Pull requests

We accept [github pull requests](https://github.com/01org/clr-oci-runtime/pulls).

## Issue tracking

If it's a bug not already documented, by all means please [open an
issue in github](https://github.com/01org/clr-oci-runtime/issues/new) so we all get
visibility on the problem and work toward resolution.

## Closing issues

You can either close issues manually by adding the fixing commit SHA1 to the issue
comments or by adding the `Fixes` keyword to your commit message:

```
Fix handling of semvers with only a single pre-release field

Fixes #121

Signed-off-by: James Hunt <james.o.hunt@intel.com>
```

Github will then automatically close that issue when parsing the
[commit message](https://help.github.com/articles/closing-issues-via-commit-messages/).