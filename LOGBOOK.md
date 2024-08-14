## logbook

here, i journal what i did in my investigation to stuff lua into sqlite.

### fri aug 9th, 2024

checked out sqlite3 git mirror, installed CLion. ensured i could build the amalgamation file correctly.
found a stackoverflow question on compiling an extension into sqlite directly, and a note in the sqlite docs
(https://sqlite.org/src/doc/trunk/README.md, https://stackoverflow.com/questions/30898113/how-to-compile-an-extension-into-sqlite)
stating that /src/shell.c contains the source to the command line program (`sqlite3`), and that this is the place where you can include extension files into the process that creates the amalgamation file.

let's see if that works.

#### attempt 1

Line 1184 onwards (git commit hash f2b21a5f57e1a1db1a286c42af40563077635c3d) contains the following bit:


    /*
    ** The source code for several run-time loadable extensions is inserted
    ** below by the ../tool/mkshellc.tcl script.  Before processing that included
    ** code, we need to override some macros to make the included program code
    ** work here in the middle of this regular program.
    */

... followed by a load of includes to what i assume is exactly what I want: a run-time loadable extension.

interestingly, serendipitously, immediately below that on line 1245 onwards is something marked as 


    /*
    ** State information for a single open session
    */

... which might be very relevant to the plan i have right now: 

- upon starting a session, create a lua state (`luaL_newstate()` i think you call to initialize something like that)
- lua functions are registered and persisted into a system table, can be something simple (function_name, function_lua_code); 
- every time the user calls lua code in their sql (SELECT calculate_something_cool_with_my_lua_udf(colname)...), this is fetched from the table and lazily loaded into the lua state.

Not to get ahead of ourselves, lets try to compile one of the sample functions (ROT13 maybe) into a sqlite binary
Then, lets compile a dummy function that prints something to the console. (`SELECT print_it();`, prints something to stdout).
Then, lets compile a dummy function that always returns '420' (`SELECT blaze_it();`, returns integer 420).

Anyway the results: just adding /ext/misc/rot13.c to the list of includes doesn't do shit.

#### Attempt 2

Looking at another extension that is known to work out of the box (the autocomplete extension thing in 'completion.c'), I see an init function sqlite3_completion_init(), 
which is called also by `shell.c.in`, line 5387, in a block of things that seem to initialize a load of functions.

Adding     sqlite3_rot_init(p->pd, 0, 0);        on line 5375... it works!!!

More next time. 

#### Attempt 3

ChatGPT tells me to just install the lua header files on my system and refer to them when building the binary, with some gcc flags.
The makefile has a macro to add extra compiler flags, so I link to the include folder (-I) and the libraries (-L) and something else thats needed (-llua, links liblua.so; might be -llua5.4 as well depending on your os):

    CFLAGS="-I /opt/homebrew/Cellar/lua/5.4.6/include/lua/ -L/opt/homebrew/Cellar/lua/5.4.6/lib/ -llua"

Can you believe adding this extension works on the first try?! See `./ext/misc/lua.c`.

I also tried compiling a standalone extension which can be loaded in the sqlite3 CLI with `.load lua`. 
The M3 macbook refused, some xcode shit I can't seem to fix; Zalgo did it however after installing `apt install tcl tcl8.6-dev lua5.4 liblua5.4-dev`, with

    gcc -shared -g -fPIC ./lua.c -I ./build/ -I ./src -I /usr/include/lua5.4 -L/usr/include/lua5.4/lualib.h -llua5.4 -o lua.so

Whooooo! Remember the 1-based indexing ;)

    sqlite> select lua('return args[1] + 1', 41); 
    42
