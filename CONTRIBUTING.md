# Contributing

Please follow the rules below if you wish to contribute to MoonLight:

## Ensure properly-formatted code

Before committing your code run `clang-format`. There is a `.clang-format`
rules file in the root directory. To run `clang-format` do:

```console
clang-format -i -style=file src/*.{cpp,h}
```

## Documentation

Please write Doxygen-style documentation in the source source.

## Python code

Please run `pylint` over your Python code and make a reasonable attempt to fix
as many issues as possible before committing.

[Back to main page](README.md)
