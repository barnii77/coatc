static char const *invalid_usage_msg = "coatc fatal: Invalid usage; use option --help or -h for more information\n";
// ----------------------
static char const *help_msg = R"(Usage: coatc [options] <input_files...> [-o <output_file>] [-O <optimization_level>]

Options:
  <input_files...>         One or more input files to compile. The files must have a `.ct` extension.
  
  -o <output_file>         Specifies the name of the output file. If not provided, the compiler will default 
                           to a name derived from the first input file.
  
  -O <optimization_level>  Sets the optimization level. Valid values are:
                             -O0  No optimization (default).
                             -O1  Basic optimization.
                             -O2  Further optimization, including inlining and loop transformations.
                             -O3  Maximum optimization, including aggressive inlining and vectorization.
                             
  -h, --help               Show this help message and exit.

Description:
  The `coatc` compiler takes one or more input files with the `.batc` extension and compiles them into a single 
  assembly output file. The output file can be specified using the `-o` option (followed by a space and the path). 
  The optimization level can be set with the `-O` option, ranging from `-O0` (no optimization) to `-O3` (maximum optimization).

Examples:
  coatc file1.batc file2.batc -o output -O3
    Compiles `file1.ct` and `file2.batc` into `output` with the highest optimization level (-O3).

  coatc -Os file1.batc -o out.o
    Compiles `file1.ct` into `out.o` with strong optimization focused on size (-Os).
)";
