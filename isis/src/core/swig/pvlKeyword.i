%module(package="isispvl") PvlKeyword
%{
    #include "PvlKeyword.h"
%}


%include "PvlKeyword.h"

%extend Isis::PvlKeyword{
  PvlKeyword(const char* key, const char* val){
            QString qkey(key);
            QString qval(val);
            Isis::PvlKeyword *kw = new Isis::PvlKeyword(qkey, qval);
            return kw;
  }
}
