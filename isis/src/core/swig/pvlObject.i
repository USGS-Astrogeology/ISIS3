%module(package="isispvl") PvlObject
%{
    #include "PvlObject.h"
%}

%include "PvlObject.h"
%nodefaultdtor Isis::PvlObject;
