@set SCIS_TXL=../txl_grammar.txl
@set SCIS_RULESET=../ruleset_grammar.txl

@set SRC=./test_source.dpr
@set DST=./test_source_instrumented.dpr
@set RULESET=./add_logging_to_Main_main_[separated_languages].yml
@set ANNOTATION=./lang/delphi/annotation.xml
@set FRAGMENTS=./fragments/delphi/

@"../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
