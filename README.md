Given a file (or a directory containing a file), VXTranslatorHelper scans its contents and reports any problems it finds (by creating two "log" files - one with outer errors, other with the errors inside quotation marks).

Take a look at these lines:
```
    Name = "Attack
    Name = Attack"
    Description = "(info> Useful item"
    Description = "(info] Useful item"
    ShowText(["You obtain \\ia[4062!"])
```
If you feel that the lines above seem to be missing opening or closing quotes, have inconsistent or missing parenthesis/brackets, and you would like something to pinpoint such problems to you - this project is made for such purpose.

Created to ease revising altered translation files.
Made to conform with https://github.com/AhmedAhmedEG/VXAceTranslator output/input (not to 100% conformance, but I hope it provides at least bare minimum)

Usage:
```
  VXTranslatorHelper INPUT_FILE_OR_DIR [INPUT_FILE_OR_DIR2 [...]]
```
  (or just drag the files that need checking onto the executable)

Outputs: files "out_err.log" and "out_inn_err.log" that pinpoint locations and types of errors, if any, in each input file 
(in format 
```
  "----------"
  file_name
  error1
  error2
  ...
```
).
Output files may be empty if no errors are found.
