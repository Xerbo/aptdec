# How To Contribute

Thanks for showing an interest in improving aptdec, these guidelines outline how you should go about contributing to the project.

## Did you find a bug?

1. Make sure the bug has not already been reported
2. If an issue already exists, leave a thumbs up to "bump" the issue
3. If an issue doesn't exist, open a new one with a clear title and description and, if relevant, any files that cause the behavior

## Do you have a patch that fixes a bug/adds a feature?

1. Fork this repository and put your changes on a *new branch*
2. Make sure to use sensible commit names
3. Open a new pull request into master

If you don't have a GitHub account, you can email me the patch at `xerbo (at) protonmail (dot) com`

## Coding style

The coding style of LeanHRPT is based off the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with minor modifications (see `.clang-format`), in addition to this all files should use LF line endings and end in a newline. Use American english (i.e. "color", not "colour").

## Commit message style

- Keep titles short to prevent wrapping (descriptions exist)
- Split up large changes into multiple commits
- Never use hastags for sequential counting in commits, as this interferes with issue/PR referencing
