/*
Copyright (C) 1994-2016 Lawrence Livermore National Security, LLC.
LLNL-CODE-425250.
All rights reserved.

This file is part of Silo. For details, see silo.llnl.gov.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the disclaimer below.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the disclaimer (as noted
     below) in the documentation and/or other materials provided with
     the distribution.
   * Neither the name of the LLNS/LLNL nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
"AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A  PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN  NO  EVENT SHALL  LAWRENCE
LIVERMORE  NATIONAL SECURITY, LLC,  THE U.S.  DEPARTMENT OF  ENERGY OR
CONTRIBUTORS BE LIABLE FOR  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR  CONSEQUENTIAL DAMAGES  (INCLUDING, BUT NOT  LIMITED TO,
PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS  OF USE,  DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER  IN CONTRACT, STRICT LIABILITY,  OR TORT (INCLUDING
NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT  OF THE USE  OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This work was produced at Lawrence Livermore National Laboratory under
Contract No.  DE-AC52-07NA27344 with the DOE.

Neither the  United States Government nor  Lawrence Livermore National
Security, LLC nor any of  their employees, makes any warranty, express
or  implied,  or  assumes  any  liability or  responsibility  for  the
accuracy, completeness,  or usefulness of  any information, apparatus,
product, or  process disclosed, or  represents that its use  would not
infringe privately-owned rights.

Any reference herein to  any specific commercial products, process, or
services by trade name,  trademark, manufacturer or otherwise does not
necessarily  constitute or imply  its endorsement,  recommendation, or
favoring  by  the  United  States  Government  or  Lawrence  Livermore
National Security,  LLC. The views  and opinions of  authors expressed
herein do not necessarily state  or reflect those of the United States
Government or Lawrence Livermore National Security, LLC, and shall not
be used for advertising or product endorsement purposes.
*/

#include "silo.h"
#include <math.h>
#include <stdlib.h>
#ifdef _WIN32
#include <string.h>
#endif
#include <std.c>

static void build_objs(DBfile *dbfile);

int main(int argc, char **argv)
{  
    DBfile        *dbfile;
    int         i, driver = DB_PDB;
    char        *filename = "ucd.pdb";
    int          show_all_errors = FALSE;
    int          allow_long_str_components = FALSE;

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i], "DB_PDB", 6)) {
            driver = StringToDriver(argv[i]);
            filename = "ucd.pdb";
        } else if (!strncmp(argv[i], "DB_HDF5", 7)) {
            driver = StringToDriver(argv[i]);
            filename = "ucd.h5";
        } else if (!strcmp(argv[i], "show-all-errors")) {
            show_all_errors = TRUE;
        } else if (!strcmp(argv[i], "allow-long-str-components")) {
            allow_long_str_components = TRUE;
	} else if (argv[i][0] != '\0') {
            fprintf(stderr, "%s: ignored argument `%s'\n", argv[0], argv[i]);
        }
    }

    DBShowErrors(show_all_errors?DB_ALL_AND_DRVR:DB_ALL, NULL);
    DBSetAllowLongStrComponents(allow_long_str_components);

    dbfile = DBCreate(filename, 0, DB_LOCAL, "ucd test file", driver);
    printf("Creating file: '%s'...\n", filename);
    build_objs(dbfile);
    DBClose(dbfile);

    CleanupDriverStuff();
    return 0;
}

void
build_objs(DBfile *dbfile)
{  DBobject *o;

   if (dbfile != NULL)
   {
      if ((o=DBMakeObject("first",DB_USERDEF,12)) != NULL)
      {  int got;

	 DBAddIntComponent(o,"member_0",0);
	 DBAddIntComponent(o,"member_1",1);
	 DBAddStrComponent(o,"member_2","two");
	 DBAddIntComponent(o,"member_3",3);
	 DBAddFltComponent(o,"member_4",4.4);
	 DBAddVarComponent(o,"member_5","five");
	 DBAddIntComponent(o,"member_6",6);
	 DBAddFltComponent(o,"member_7",7.7);
	 DBAddVarComponent(o,"member_8","eight");
	 DBAddIntComponent(o,"member_9",9);
         DBAddDblComponent(o,"member_10",10.10101010101010);
         DBAddStrComponent(o,"member_11",
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "VeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVeryVery"
             "LongString");
	 got = DBWriteObject(dbfile,o,0);
	 DBFreeObject(o);
	 if (got < 0)
	    goto punt;
	 if ((o=DBMakeObject("second",DB_USERDEF,8)) != NULL)
	 {  DBAddStrComponent(o,"field_0","zero");
	    DBAddStrComponent(o,"field_1","one");
	    DBAddStrComponent(o,"field_2","two");
	    DBAddIntComponent(o,"field_3",3);
	    DBAddIntComponent(o,"field_4",4);
	    DBAddDblComponent(o,"field_5",55555.5555555555);
	    DBAddFltComponent(o,"field_6",6.6);
	    DBAddIntComponent(o,"field_7",7);
	    got = DBWriteObject(dbfile,o,0);
	    DBFreeObject(o);
	    if (got < 0)
	       goto punt;
	    if ((o=DBMakeObject("third",DB_USERDEF,4)) != NULL)
	    {  DBAddIntComponent(o,"component_0",0);
	       DBAddVarComponent(o,"component_1","one");
	       DBAddIntComponent(o,"component_2",2);
	       DBAddIntComponent(o,"component_3",3);
	       got = DBWriteObject(dbfile,o,0);
	       DBFreeObject(o);
	       if (got < 0)
		  goto punt;
	       if ((o=DBMakeObject("third",DB_USERDEF,5)) != NULL)
	       {  DBAddStrComponent(o,"part_0","zero");
		  DBAddVarComponent(o,"part_1","one");
		  DBAddStrComponent(o,"part_2","two");
		  DBAddIntComponent(o,"part_3",3);
		  DBAddFltComponent(o,"part_4",4.4);
		  got = DBWriteObject(dbfile,o,0);
		  DBFreeObject(o);
		  if (got < 0)
		     goto punt;
	       }
	    }
	 }
      }
punt:
    ; 
   }
}

