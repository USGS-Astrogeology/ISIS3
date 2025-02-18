#include "Fixtures.h"

namespace Isis {

  void RawPvlKeywords::SetUp() {
    keywordsToTry = {
      "KEYWORD",
      "KEYWORD X",
      "KEYWORD =",
      "KEYWORD = SOME_VAL",
      "KEYWORD = \"  val  \"",
      "KEYWORD = \" 'val' \"",
      "KEYWORD = (VAL",
      "KEYWORD = (VAL1,VAL2",
      "KEYWORD = (A B,C,D)",
      "KEYWORD = ((A B),(C),(D",
      "KEYWORD = (SOME_VAL)",
      "KEYWORD = (SOME_VAL) <a>",
      "KEYWORD=(SOME_VAL)<a>",
      "KEYWORD = (A, )",
      "KEYWORD = ()",
      "KEYWORD = (A,B)",
      "KEYWORD = {A, B}",
      "KEYWORD = (A,B) #comment this",
      "KEYWORD = ( A , B )",
      "KEYWORD\t=\t( A\t,\tB )",
      "KEYWORD = (A, B,C,D,E))",
      "KEYWORD = ((1, 2), {3,  4}, (5), 6)",
      "KEYWORD = { \"VAL1\" ,   \"VAL2\", \"VAL3\"}",
      "KEYWORD = { \"VAL1\" , \"VAL2\", \"VAL3\")",
      "KEYWORD = { \"VAL1\" ,",
      "KEYWORD = \"(A,B,\"",
      "KEYWORD = ',E)'",
      "KEYWORD = ((1,2))",
      "KEYWORD = (\"(f1+f2)\",\"/(f1-f2)\")",
      "KEYWORD = \"(F1+F2)/(F1-F2)\"",
      "KEYWORD = ( (1,2)  , (A,B) )",
      "KEYWORD = \"(f1 + min(f2,f3))\"",
      "KEYWORD = \"(min(f2,f3) + f1)\"",
      "KEYWORD = \"min(f2,f3) + f1\"",
      "KEYWORD = \"f1 + min(f2,f3)\"",
      "KEYWORD = (A <a>, B <b>, C, D <d>)",
      "KEYWORD = (A <a>, B <b>, C, D <d>) <e>",
      "KEYWORD = ',E) <unit>",
      "KEYWORD = ,E) <unit>",
      "#SOMECOMMENT\nKEYWORD = SOME_VAL",
      "#SOMECOMMENT1\n#SOMECOMMENT2\nKEYWORD = SOME_VAL",
      "//SOMECOMMENT1\n#SOMECOMMENT2\nKEYWORD = SOME_VAL",
      "/*SOMECOMMENT1*/\nKEYWORD = SOME_VAL",
      "KEYWORD = '/*\n*/'",
      "/* SOMECOMMENT1\n  SOMECOMMENT2\nSOMECOMMENT3 */\nKEYWORD = SOME_VAL",
      "/*C1\n\nA\n/*\nC3*/\nKEYWORD = SOME_VAL",
      "/*C1\n/**/\nKEYWORD = SOME_VAL",
      "/*C1\nA/**/\nKEYWORD = SOME_VAL",
      "/*           A            */\n/* B *//*C*/\nKEYWORD = SOME_VAL",
      "/*C1/**/\nKEYWORD = SOME_VAL",
      "/*C1   \n\nA\n\nC3*//*Neato*//*Man*/KEYWORD = (A,B,C) /*Right?\nYes!*/"
    };

    results = {
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 0
      PvlKeyword("KEYWORD", "  val  "),           // 1
      PvlKeyword("KEYWORD", "  \'val\'  "),       // 2
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 3
      PvlKeyword("KEYWORD", "SOME_VAL", "a"),     // 4
      PvlKeyword("KEYWORD", "SOME_VAL", "a"),     // 5
      PvlKeyword("KEYWORD"),                      // 6
      PvlKeyword("KEYWORD"),                      // 7
      PvlKeyword("KEYWORD"),                      // 8
      PvlKeyword("KEYWORD"),                      // 9
      PvlKeyword("KEYWORD"),                      // 10
      PvlKeyword("KEYWORD"),                      // 11
      PvlKeyword("KEYWORD"),                      // 12
      PvlKeyword("KEYWORD"),                      // 13
      PvlKeyword("KEYWORD", "(A,B,"),             // 14
      PvlKeyword("KEYWORD", ",E)"),               // 15
      PvlKeyword("KEYWORD", "(1,2)"),             // 16
      PvlKeyword("KEYWORD"),                      // 17
      PvlKeyword("KEYWORD", "(F1+F2)/(F1-F2)"),   // 18
      PvlKeyword("KEYWORD"),                      // 19
      PvlKeyword("KEYWORD", "(f1 + min(f2,f3))"), // 20
      PvlKeyword("KEYWORD", "(min(f2,f3) + f1)"), // 21
      PvlKeyword("KEYWORD", "min(f2,f3) + f1"),   // 22
      PvlKeyword("KEYWORD", "f1 + min(f2,f3)"),   // 23
      PvlKeyword("KEYWORD"),                      // 24
      PvlKeyword("KEYWORD"),                      // 25
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 26
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 27
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 28
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 29
      PvlKeyword("KEYWORD", "/*\n*/"),            // 30
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 31
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 32
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 33
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 34
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 35
      PvlKeyword("KEYWORD", "SOME_VAL"),          // 36
      PvlKeyword("KEYWORD"),                      // 37
    };

    results[7].addValue("A");
    results[7].addValue("B");

    results[8].addValue("A");
    results[8].addValue("B");

    results[9].addValue("A");
    results[9].addValue("B");
    results[9].addComment("#comment this");

    results[10].addValue("A");
    results[10].addValue("B");

    results[11].addValue("A");
    results[11].addValue("B");

    results[12].addValue("(1, 2)");
    results[12].addValue("{3, 4}");
    results[12].addValue("(5)");
    results[12].addValue("6");

    results[13].addValue("VAL1");
    results[13].addValue("VAL2");
    results[13].addValue("VAL3");

    results[17].addValue("(f1+f2)");
    results[17].addValue("/(f1-f2)");

    results[19].addValue("(1,2)");
    results[19].addValue("(A,B)");

    results[24].addValue("A", "a");
    results[24].addValue("B", "b");
    results[24].addValue("C");
    results[24].addValue("D", "d");

    results[25].addValue("A", "a");
    results[25].addValue("B", "b");
    results[25].addValue("C", "e");
    results[25].addValue("D", "d");

    results[26].addComment("#SOMECOMMENT");

    results[27].addComment("#SOMECOMMENT1");
    results[27].addComment("#SOMECOMMENT2");

    results[28].addComment("//SOMECOMMENT1");
    results[28].addComment("#SOMECOMMENT2");

    results[29].addComment("/* SOMECOMMENT1 */");

    results[31].addComment("/* SOMECOMMENT1 */");
    results[31].addComment("/* SOMECOMMENT2 */");
    results[31].addComment("/* SOMECOMMENT3 */");

    results[32].addComment("/* C1 */");
    results[32].addComment("/* A  */");
    results[32].addComment("/*    */");
    results[32].addComment("/* C3 */");

    results[33].addComment("/* C1  */");
    results[33].addComment("/*     */");

    results[34].addComment("/* C1  */");
    results[34].addComment("/* A/* */");

    results[35].addComment("/*           A            */");
    results[35].addComment("/* B *//*C                */");

    results[36].addComment("/* C1/* */");

    results[37].addValue("A");
    results[37].addValue("B");
    results[37].addValue("C");
    results[37].addComment("/* C1    */");
    results[37].addComment("/* A     */");
    results[37].addComment("/* C3    */");
    results[37].addComment("/* Neato */");
    results[37].addComment("/* Man   */");
    results[37].addComment("/*Right? Yes!*/");

    valid = {
      false,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      false,
      false,
      true,
      true,
      true,
      false,
      true,
      true,
      true,
      true,
      true,
      true,
      false,
      true,
      true,
      false,
      false,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      false,
      false,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true};
  }

  void RawPvlKeywords::TearDown() {}
}