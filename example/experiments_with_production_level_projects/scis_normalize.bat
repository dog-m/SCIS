@echo === C++ ==========

@set TXL=../lang/cpp/grammar.txl
@set SRC=./samples/[boost]_uuid-develop-include-boost-uuid-detail-sha1.hpp
@set DST=./_normalized.hpp

@txl %SRC% %TXL% -o %DST%

@echo === delphi =======

@set TXL=../lang/delphi/grammar.txl
@set SRC=./samples/[lazarus]_ide-compiler.pp
@set DST=./_normalized.pas

@txl %SRC% %TXL% -o %DST%

@echo === java =========

@set TXL=../lang/java/grammar.txl
@set SRC=./samples/[aspectj]_asm-src-main-java-org-aspectj-asm-AsmManager.java
@set DST=./_normalized.java

@txl %SRC% %TXL% -o %DST%

@echo === python =======

@set TXL=../lang/python/grammar.txl
@set SRC=./samples/[keras]_keras-layers-convolutional.py
@set DST=./_normalized.py

@txl %SRC% "..\lang\python\pyindent.txl" | txl stdin %TXL% -o %DST%

@pause
