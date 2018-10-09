<?php


/**
 *  Generate Random password contains alphabets and numbers
 */
function tripal_apollo_gen_xkcd_password($num_of_words = 1) {
  $dict_lines = file('/usr/share/dict/words', FILE_IGNORE_NEW_LINES);
  $length = count($dict_lines);
  $password = '';
  $rand_num = rand(1, 9);
  for ($i = 1; $i <= $num_of_words; $i++) {
    $plain = FALSE;

    while (!$plain) {
      // Get random word from $lines
      $key = mt_rand(0, $length);
      if ((preg_match("/^[a-z]+$/", $dict_lines[$key]) == 1) && (strlen($dict_lines[$key]) < 9)) {
        // String only contains a to z characters
        $plain = TRUE;
        $password = $password . ucwords($dict_lines[$key]);

        if ($i != $num_of_words) {
          $password = $password . $rand_num;
        }
      }
    }
  }
  return $password;
}