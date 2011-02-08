dnl
dnl @file orchids-rulegen.m4
dnl Orchids Automatic Rule Generator
dnl 
dnl @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
dnl 
dnl @version 0.1.0
dnl 
dnl @date  Started on: Mon Dec  1 01:01:06 2003
dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl Initialise counter variable
define(`count_val', 0)dnl
dnl
dnl --------------------------------------------------------------------------
dnl Subsitute to counter value, and redefine to the next value
define(`counter', `count_val`'define(`count_val', incr(count_val))')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl pattern( name , cond )
define(`pattern', `define(`pattern_'$1, $2)dnl')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl actions( actionblockname , actions )
define(`actions', `define(`actions_'$1, $2)dnl')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
define(`expand_actions', `ifdef(actions_`'$1,actions_`'$1, `  /* no $1 action */')')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl ruleheader( rulename )
define(`ruleheader',`dnl
/*
** Rule $1
** Automatically generated by Rulegen
** on: esyscmd(date)dnl
*/

')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl rule( name , rule [ , actionbefore [ , actionafter ] ] )
define(`rule', `dnl
ruleheader($1)
`rule' $1 `{
  state init {'
expand_actions($3)
$2
`    goto end;
  }
  state end {'
expand_actions($4)
`    /* end of rule */
  }
}'')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl event( pattern [ , actionbefore [ , actionafter ] ] )
define(`event', `dnl
define(`id', eval(counter))dnl
    goto event_`'id`'_start;
  }

  state event_`'id`'_start {
expand_actions($2)
    if ( pattern_$1 )
      goto event_`'id`'_end;
  }

  state event_`'id`'_end {
expand_actions($3)')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl before( expr1 , expr2 [ , actionbefore [ , actionafter ] ] )
define(`before', `dnl
define(`id', eval(counter))dnl
    goto before_`'id`'_start;
  }

  state before_`'id`'_start {
expand_actions($3)
$1
$2
    goto before_`'id`'_end;
  }

  state before_`'id`'_end {
expand_actions($4)')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl after( expr1, expr2 [ , actionbefore [ , actionafter ] ] )
define(`after', `dnl
define(`id', eval(counter))dnl
    goto after_`'id`'_start;
  }

  state after_`'id`'_start {
expand_actions($3)
$2
$1
    goto after_`'id`'_end;
  }

  state after_`'id`'_end {
expand_actions($4)')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl repeat( expr , value [ , actionbefore [ , actionafter ] ] )
define(`repeat', `dnl
define(`id', eval(counter))dnl
    goto repeat_`'id`'_start;
  }

  state repeat_`'id`'_start {
expand_actions($3)
    $repeat_`'id`'_count = 0;
    goto repeat_`'id`'_loop;
  }

  state repeat_`'id`'_loop {
    if ( $repeat_`'id`'_count >= eval($2) )
      goto repeat_`'id`'_end;
    if ( $repeat_`'id`'_count < eval($2) )
      goto repeat_`'id`'_begin;
  }

  state repeat_`'id`'_begin {
$1
    $repeat_`'id`'_count = $repeat_`'id`'_count + 1;

    goto repeat_`'id`'_loop;
  }

  state repeat_`'id`'_end {
expand_actions($4)')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl oneamong( expr1 , expr2 [ , actionbefore [ , actionafter ] ] )
define(`oneamong', `dnl
define(`id', eval(counter))dnl
    goto oneamong_`'id`'_start;
  }

  state oneamong_`'id`'_start {
expand_actions($3)
    goto oneamong_`'id`'_start_1;
    goto oneamong_`'id`'_start_2;
  }

  state oneamong_`'id`'_start_1 {
$1
    goto oneamong_`'id`'_end;
  }

  state oneamong_`'id`'_start_2 {
$2
    goto oneamong_`'id`'_end;
  }

  state oneamong_`'id`'_end {
expand_actions($4)')dnl
dnl
dnl
dnl --------------------------------------------------------------------------
dnl without( expr1 , expr2 [ , actionbefore [ , actionafter ] ] )
define(`without', `dnl
define(`id', eval(counter))dnl
    goto without_`'id`'_start;
  }
  state without_`'id`'_start {
expand_actions($3)
    goto without_`'id`'_start_1;
    goto without_`'id`'_start_2;
  }

  state without_`'id`'_start_1 {
$1
    cut("without_`'id`'_start");

    goto without_`'id`'_end;
  }

  state without_`'id`'_start_2 {
$2
    cut("without_`'id`'_start");
  }

  state without_`'id`'_end {
expand_actions($4)')dnl
