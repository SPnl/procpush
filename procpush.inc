<?php

/**
 * Checks wether a submission contains sufficient data to create new or sync existing civi contact.
 */
function procpush_check_submission_sufficient($data) {
  // Check if only metadata.
  $data_copy = $data;
  $data_fields = procpush_data_fields();
  foreach ($data_fields as $field_id => $field_properties) {
    if ($field_properties['meta']) {
      unset($data_copy[$field_id]);
    }
  }
  if (empty($data_copy)) {
    return FALSE;
  }

  // Check if values are sufficient to create new or sync existing civi contact.
  if (!empty($data['relation_id'])) {
    return TRUE;
  }

  if ((!empty($data['last_name']) && $data['last_name'] !== '.') || !empty($data['first_name'])) {
    if (!empty($data['email']) || !empty($data['fixed_phone']) || !empty($data['mobile_phone']) || procpush_check_address_sufficient($data)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Create procurios form submission object from webformsync data.
 */
function procpush_create_data_object($data) {
  // Simple text field mappings are set in admin form.
  $mapping = variable_get('procpush_field_mapping');
  $object = array();
  foreach ($mapping as $webformsync_key => $procurios_key) {
    $keys = explode('|', $procurios_key);
    if (isset($data[$webformsync_key])) {
      $md = array();
      $md[$keys[count($keys)-1]] = $data[$webformsync_key];
      for($i=count($keys)-2; $i>-1; $i--) {
        $md[$keys[$i]] = $md;
        unset($md[$keys[$i+1]]);
      }
      $object = array_merge_recursive($object, $md);
    }
  }

  // Get form schema to validate field values.
  $api_schema_raw = procapi_registration_get_form_schema();
  $api_properties = $api_schema_raw['properties'];

  // SP Actief.
  if (!empty($data['sp_active'])) {
    // Actief SP.
    if (!empty($data['sp_active']['active'])) {
      if (isset($data['sp_active']['active'])) {
        $api_key = variable_get('procpush_field_sp_active_active_mapping');
        if (!empty($api_key)) {
          $object[$api_key] = $data['sp_active']['active'];
        }
      }
    }
    // Activiteiten SP.
    if (!empty($data['sp_active']['activities'])) {
      $api_key = variable_get('procpush_field_sp_active_activities_mapping');
      if (!empty($api_key)) {
        foreach ($data['sp_active']['activities'] as $activity) {
          if (isset($api_properties[$api_key]['properties'][$activity])) {
            $object[$api_key][$activity] = TRUE;
          }
        }
      }
    }
  }
  
  // SP Work and Interests field mapping.
  if (!empty($data['sp_work_and_int'])) {
    // Hoofdtaak.
    if (!empty($data['sp_work_and_int']['main_task'])) {
      $api_key = variable_get('procpush_field_sp_work_and_int_main_task_mapping');
      if (!empty($api_key)) {
        foreach ($data['sp_work_and_int']['main_task'] as $task) {
          if (isset($api_properties[$api_key]['properties'][$task])) {
            $object[$api_key][$task] = TRUE;
          }
        }
      }
    }
    // Bedrijfstak.
    if (!empty($data['sp_work_and_int']['industrial_sector'])) {
      $api_key = variable_get('procpush_field_sp_work_and_int_industrial_sector_mapping');
      if (!empty($api_key)) {
        $sector = str_replace(', ', '_', $data['sp_work_and_int']['industrial_sector']);
        if (isset($api_properties[$api_key]['properties'][$sector])) {
          $object[$api_key][$sector] = TRUE;
        }
      }
    }
    // Beroepsgroep.
    if (!empty($data['sp_work_and_int']['occupational_group'])) {
      $api_key = variable_get('procpush_field_sp_work_and_int_occupational_group_mapping');
      if (!empty($api_key)) {
        if (isset($api_properties[$api_key]['properties'][$data['sp_work_and_int']['occupational_group']])) {
          $object[$api_key][$data['sp_work_and_int']['occupational_group']] = TRUE;
        }
      }
    }
  }

  // Privacy.
  if (!empty($data['privacy'])) {
    $api_key = variable_get('procpush_field_privacy_mapping');
    if (!empty($api_key)) {
      foreach ($data['privacy'] as $privacy_option) {
        if (isset($api_properties[$api_key]['properties'][$privacy_option])) {
          $object[$api_key][$privacy_option] = TRUE;
        }
      }
    }
  }

  // Overwrite.
  if (isset($data['overwrite'])) {
    $api_key = variable_get('procpush_field_overwrite_mapping');
    if (!empty($api_key)) {
      $object[$api_key] = empty($data['overwrite']) ? FALSE : TRUE;
    }
  }
  return $object;
}

/*
 * Mail error to tdgraaff.
 */
function procpush_error($error_message, $debug_data) {
  global $base_url;
  $message = array(
    'content' => '<p>' . $error_message . '</p><p><pre>Debug data:<pre>@message_data</pre></p>',
    'data' => array(
      '@message_data' => print_r($debug_data, TRUE),
    ),
  );
  watchdog('procpush', $message['content'], $message['data'], WATCHDOG_ERROR);
  drupal_mail('spwebformsync', 'error', 'tdgraaff@sp.nl', language_default(), $message, 'webmaster@sp.nl');
}

function procpush_data_fields_flat_list() {
  $flat_fields = &drupal_static(__FUNCTION__);
  if (!isset($flat_fields)) {
    $flat_fields = array();
    $fields = procpush_data_fields();
    foreach ($fields as $key => $values) {
      if ($values['type'] == 'string') {
        $flat_fields[$key] = $values['title'];
      }
    }
  }
  return $flat_fields;
}

/**
 * Add / remove relation to / from selection.
 */
function procpush_add_relation_to_selections($relation_id, $selections) {
  $error = FALSE;
  if (!empty($selections['add'])) {
    // Add contact to selections.
    foreach ($selections['add'] as $selection_id) {
      // Add relation to selection.
      if (procapi_selection_add_relations($selection_id, array($relation_id)) === FALSE) {
        $error = TRUE;
      }
    }
  }
  if (!empty($selections['remove'])) {
    // Remove contact from selections.
    foreach ($selections['remove'] as $selection_id) {
      // Remove relation to selection.
      if (procapi_selection_remove_relations($selection_id, array($relation_id)) === FALSE) {
        $error = TRUE;
      }
    }
  }
  if ($error) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Prepare data for Procurios.
 */
function procpush_fix_data(&$data) {
  // Check for remove phone number.
  if (!empty($data['overwrite'])) {
    $phone_types = array('phone', 'mobile_phone', 'fixed_phone');
    foreach ($phone_types as $key) {
      if (!empty($data[$key])) {
        if (strpos($data[$key], '[remove:') !== FALSE) {
          $phone_stripped = preg_replace("/[^0-9]/", "", $data[$key]);
          // Remove phone number
          if ($key == 'phone') {
            if (preg_match('/^(00)?(31)?0?6([1-5]|8)[0-9]{7}$/', $phone_stripped)) {
              $data['mobile_phone'] = '0600000000';
            }
            else {
              $data['fixed_phone'] = '0600000000';
            }
            unset($data['phone']);
          }
          else {
            $data[$key] = '0600000000';
          }
        }
      } 
    }
  }

  // Remove emojis.
  procpush_basic_cleanup($data);

  // Split name in first and last name when possible.
  if (!empty($data['name']) && empty($data['last_name'])) {
    $split_name = procpush_split_name($data['name']);    
    if (!empty($split_name['last_name']) || !empty($split_name['first_name'])) {
      $data['first_name'] = empty($split_name['first_name']) ? '' : $split_name['first_name'];
      $data['middle_name'] = empty($split_name['middle_name']) ? '' : $split_name['middle_name'];
      $data['last_name'] = $split_name['last_name'];
    }
  }
  unset($data['name']);

  // Split house_number_and_addition when available.
  if (!empty($data['house_number_and_addition']) && empty($data['house_number'])) {
    $split_house_number = procpush_split_house_number_and_addition($data['house_number_and_addition']);    
    if (!empty($split_house_number['house_number'])) {
      $data['house_number'] = $split_house_number['house_number'];
      $data['house_number_addition'] = $split_house_number['house_number_addition'];
    }
    else {
      $data['house_number'] = $data['house_number_and_addition'];
    }
  }
  unset($data['house_number_and_addition']);

  // Convert phone to fixed_phone or mobile_phone.
  if (!empty($data['phone'])) {
    $phone_stripped = preg_replace("/[^0-9]/", "", $data['phone']);
    if (preg_match('/^(00)?(31)?0?6([1-5]|8)[0-9]{7}$/', $phone_stripped) === 1) {
      $data['mobile_phone'] = $data['phone'];
    }
    else {
      $data['fixed_phone'] = $data['phone'];
    }
  }
  unset($data['phone']);

  // If contact number is available, and relation id is not set, try to map contact number to relation id.
  if (empty($data['relation_id']) && !empty($data['contact_number'])) {
    $relation_id = procpush_get_relation_id($data['contact_number']);
    if (!empty($relation_id)) {
      $data['relation_id'] = $relation_id;
    }
  }
  procpush_validate_data($data);
}

/**
 * Basic cleanup of data strings.
 */
function procpush_basic_cleanup(&$data) {
  $emojiregex = '%([*#0-9](?>\\xEF\\xB8\\x8F)?\\xE2\\x83\\xA3|\\xC2[\\xA9\\xAE]|\\xE2..(\\xF0\\x9F\\x8F[\\xBB-\\xBF])?(?>\\xEF\\xB8\\x8F)?|\\xE3(?>\\x80[\\xB0\\xBD]|\\x8A[\\x97\\x99])(?>\\xEF\\xB8\\x8F)?|\\xF0\\x9F(?>[\\x80-\\x86].(?>\\xEF\\xB8\\x8F)?|\\x87.\\xF0\\x9F\\x87.|..(\\xF0\\x9F\\x8F[\\xBB-\\xBF])?|(((?<zwj>\\xE2\\x80\\x8D)\\xE2\\x9D\\xA4\\xEF\\xB8\\x8F\k<zwj>\\xF0\\x9F..(\k<zwj>\\xF0\\x9F\\x91.)?|(\\xE2\\x80\\x8D\\xF0\\x9F\\x91.){2,3}))?))%';
  $fields = procpush_data_fields();
  foreach ($fields as $key => $field_info) {
    if (isset($data[$key])) {
      if ($field_info['type'] == 'string') {
        // Trim.
        $data[$key] = trim($data[$key]);
        // Remove duplicate spaces.
        $data[$key] = preg_replace('/\s+/', ' ',$data[$key]);
        // Remove emojis.
        $result = preg_replace($emojiregex, '', $data[$key]);
        if ($result !== false) {
          $data[$key] = $result;
        }
        // Remove invalid characters.
        procpush_remove_invalid_characters($data[$key]);
      }
    }
  }
}

/**
 * Return invalid characters.
 */
function procpush_remove_invalid_characters(&$string) {
  $regex = '/[?!#$%^*{}[\];:<>]/';
  $result = preg_replace($regex, '', $string);
  if ($result !== false) {
    $string = $result;
  }
}

/**
 * Split name.
 */
function procpush_split_name($name) {
  $names = array();

  procpush_cleanup_raw_name($name);

  // Split name.
  $name_split_success = FALSE;

  // Try to split name on middle name.
  $middle_names_array = procpush_middle_names();
  $middle_names_regex = implode('|', $middle_names_array);

  $matches = array();
  $regex = "@^(?'first_name'(([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-\.]+)\s*)+?)?(?'middle_name'(\b(" . $middle_names_regex . ")\b\s*))(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][a-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+)$@";
  if (preg_match($regex, $name, $matches)) {
    $names = array(
      'first_name' => empty($matches['first_name']) ? '' : trim($matches['first_name']),
      'middle_name' => empty($matches['middle_name']) ? '' : trim($matches['middle_name']),
      'last_name' => empty($matches['last_name']) ? '' : trim($matches['last_name']),
    );
    $name_split_success = TRUE;
  }
  else {
    // Split name on middle name, last name first.
    $regex = "@^(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+),?\s(?'first_name'(([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-\.]+)\s*)+?)(?'middle_name'(\b(" . $middle_names_regex . ")\b\s*))$@";
    if (preg_match($regex, $name, $matches)) {
      $names = array(
        'first_name' => empty($matches['first_name']) ? '' : trim($matches['first_name']),
        'middle_name' => empty($matches['middle_name']) ? '' : trim($matches['middle_name']),
        'last_name' => empty($matches['last_name']) ? '' : trim($matches['last_name']),
      );
      $name_split_success = TRUE;
    }
  }

  if (!$name_split_success) {
    // Split on first name(s).
    $name_parts = explode(' ', $name);
    $first_names = array();
    foreach ($name_parts as $name_part) {
      $sql = "SELECT first_name FROM procpush_first_names WHERE LOWER(first_name) = LOWER(:name_part)";
      $query = db_query($sql, array('name_part' => $name_part));
      $result = $query
        ->fetchField();
      if (!empty($result)) {
        $first_names[] = $name_part;
      }
      else {
        break;
      }
    }
    if (!empty($first_names)) {
      // First name found.
      $first_name = implode(' ', $first_names);
      $names['first_name'] = $first_name;
      $last_name = preg_replace('/^' . $names['first_name'] . '/', '', $name);
      $names['middle_name'] = '';
      $names['last_name'] = empty($last_name) ? '' : $last_name;
    }
    elseif (count($name_parts) === 1) {
      // Only single last name.
      $names['first_name'] = $name;
      $names['middle_name'] = '';
      $names['last_name'] = '';
    }
    else {
      // Split in first word and rest for last name.
      $names['first_name'] = array_shift($name_parts);
      $names['middle_name'] = '';
      $names['last_name'] = implode(' ', $name_parts);
    }
  }

  // Set first character uppercase.
  $parts = array(
    'first_name',
    'last_name',
  );
  foreach ($parts as $part) {   
    if (isset($names[$part])) {  
      if (!empty($names[$part])) {
        $names[$part] = ucwords($names[$part]);
      }
    }
  }

  return $names;
}

function procpush_cleanup_raw_name(&$name) {
  // Remove titles.
  $titles_regex = '/^(de heer|mr|mevrouw|mevr|dhr|ir|ing|bc|ms|drs|dr).? /i';
  $result = preg_replace($titles_regex, '', $name);
  $name = !empty($result) ? $result : $name;

  // Remove comma from name (Achternaam, Voornaam plus bijvoegsel).
  $comma_pos = strpos($name, ',');
  if ($comma_pos !== FALSE) {
    $first_half = trim(substr($name, 0, $comma_pos));
    $last_half = trim(substr($name, $comma_pos + 1));
    $name = $last_half . ' ' . $first_half;
  }

  // Remove whitespace and ., from front and end of name.
  $name = trim($name);
  $name = trim($name, '.,');

  // Remove duplicate whitespace.
  $name = preg_replace('!\s+!', ' ', $name);

  // Replace terms.
  $replace = array(
    'v' => 'van',
    'v.' => 'van',
    'vd' => 'van de',
    'vd.' => 'van de',
    'v.d.' => 'van de',
    'v.d' => 'van de',
    'v/d' => 'van de',
  );
  foreach ($replace as $orig => $new) {
    $name = preg_replace('@ ' . $orig . ' @i', ' ' . $new . ' ', $name);
  }
}

/**
 * Checks if the address data is sufficient.
 */
function procpush_check_address_sufficient($data) {
  // Check for Testlaan (street) 1 (house_number), Amsterdam (locality)
  if (!empty($data['street']) && !empty($data['house_number']) && !empty($data['locality'])) {
    return TRUE;
  }
  // Check for house number and postal code, for example: 1234AX 7
  // This is sufficient because we can determine the street and city
  // from the postal code.
  if (!empty($data['house_number']) && !empty($data['postal_code'])) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Split house number.                                                                                                                       
 */
function procpush_split_house_number_and_addition($house_number) {
  $house_number = trim($house_number);
  $regex = "@^([0-9]+)([^0-9].*)$@";
  if (preg_match($regex, $house_number, $matches)) { 
    $split_house_number['house_number'] = trim($matches[1]);
    $split_house_number['house_number_addition'] = trim($matches[2]);
    if (!empty($split_house_number['house_number'])) {
      return $split_house_number;
    }
  }
  return FALSE;
}

/**
 * Longer middle names first.
 */
function procpush_middle_names() {
  return array("Uijt te de","van van de","uijt te de","Voor in ‘t","De van der","de van der","Van van de","voor in ‘t","Uit te de","onder den","Voor in t","onder het","van de l’","De die le","de die le","Van de l’","voor in t","Onder het","Onder den","uit te de","over het","uijt den","Onder de","boven d’","uijt ten","Uijt ten","over den","Over den","onder de","onder ‘t","Onder ‘t","Van de l","Boven d’","van de l","voor den","Over het","Uijt den","Voor den","Over ‘t","Boven d","Von dem","boven d","Von der","von den","Von den","von der","von dem","Onder t","van der","Van ter","Van gen","van het","van gen","Van het","van ter","over ‘t","van den","Van den","Van Der","Van der","onder t","Van Den","bij den","auf dem","Auf den","Auf dem","voor de","uit het","Uit het","auf den","Auf der","Voor de","voor ‘t","auf ter","Auf ter","auf der","aan het","Aan het","uijt de","vor der","Uijt de","uijt ‘t","Uijt ‘t","Vor der","Uit ten","Aan der","aan der","aan den","Aan den","uit ten","uit den","Uit den","aus der","Bij het","Bij den","Voor ‘t","over de","Over de","bij het","Aus dem","aus dem","Aus den","Aus der","aus den","in het","die le","In der","in der","In het","Uit de","van ‘t","Van de","van De","van de","Van ‘t","Uit ‘t","In den","uit de","uit ‘t","in den","Op ten","op het","Over t","Op het","Die le","op gen","over t","op ten","Von ‘t","Bij ‘t","aus ‘m","bij ‘t","Bij de","von ‘t","bij de","Op gen","op der","Aan ‘t","aan ‘t","De las","de las","Van la","van la","Aan de","aan de","op den","Op der","De die","de die","Op den","Aus ‘m","Onder","op ‘t","Op ‘t","Op de","op de","onder","Uit t","Van t","De l’","uit t","Dalla","de l’","De la","de le","De le","de la","Von t","bij t","Am de","aan t","Aan t","am de","Aus m","Bij t","von t","aus m","Degli","dalla","van t","In de","in ‘t","In ‘t","della","in de","unter","Unter","Della","degli","deca","uijt","Over","over","Voor","in t","In t","Uijt","voor","op t","De l","thoe","de l","thor","Deca","Thor","Dal’","Thoe","Op t","dal’","zum","Zur","toe","zur","tot","Toe","Tot","uit","vom","Von","Vom","ver","Ver","van","Van","vor","Vor","Uit","von","Zum","Los","Den","den","del","Del","Dei","dei","Der","der","dos","Het","Dos","don","Des","des","das","Das","Aus","aus","auf","Auf","Aan","aan","Ben","ben","Dal","dal","bin","Bin","Bij","bij","het","Don","Ter","las","ter","ten","Ten","los","les","Les","Tho","Las","tho","d’","‘T","‘t","D’","Da","da","l’","op","S’","‘s","s’","te","af","am","Am","al","Af","de","Al","zu","Zu","to","Te","De","du","El","Du","Le","do","Op","el","la","in","L’","In","im","La","Im","Do","To","lo","di","‘S","Of","Lo","le","of","Di","A","l","S","s","L","I","T","t","D","d","i","a");
}



/**
 * Validates data.
 */
function procpush_validate_data(&$data) {
  $fields = procpush_data_fields();
  foreach ($fields as $key => $field_info) {
    if (!empty($data[$key])) {
      // Validate data with special function.
      if (function_exists('procpush_valid_' . $key)) {
        $data[$key] = call_user_func('procpush_valid_' . $key, $data[$key]);
      }
    }
    if (empty($data[$key])) {
      unset($data[$key]);
    }
  }
}

/**
 * Check if relation id is valid.
 */
function procpush_valid_relation_id($relation_id) {
  $relation_id = preg_replace('/[^0-9]/s', '', $relation_id);
  if (!empty($relation_id)) {
    if (!is_numeric($relation_id) || intval($relation_id) != $relation_id || $relation_id <= 0) {
      $relation_id = '';
    }
  }
  return $relation_id;
}

/**
 * Check if first name is valid.
 */
function procpush_valid_first_name($first_name) {
  $first_name = ucwords(strtolower($first_name), " -\t\r\n\f\v");
  return $first_name;
}

/**
 * Check if middle name is valid.
 */
function procpush_valid_middle_name($middle_name) {
  $middle_name = strtolower($middle_name);
  return $middle_name;
}

/**
 * Check if last name is valid.
 */
function procpush_valid_last_name($last_name) {
  $last_name = ucwords(strtolower($last_name), " -\t\r\n\f\v");
  return $last_name;
}

/**
 * Check if email is valid.
 */
function procpush_valid_email($email) {
  if (!valid_email_address($email)) {
    $email = '';
  }
  $email = strtolower($email);
  return $email;
}

/**
 * Check if phone is valid.
 */
function procpush_valid_telephone($telephone) {
  $number_only = preg_replace('@[^0-9]@', '', $telephone);
  if (preg_match('/^((00)?31)?0?[1-9][0-9]{8}$/', $number_only)) {
    // Waarschijnlijk Nederlands telefoonnummer.
    // Remove non numerical characters.
    $telephone = preg_replace('@[^0-9]@', '', $telephone);

    // 612345678 => 012345678
    if (strlen($telephone) === 9) {
      $telephone = '0' . $telephone;
    }
    // 31612345678 => 0649492174
    if (
      strlen($telephone) === 11 &&
      strpos($telephone, '31') === 0
    ) {
      $telephone = substr_replace($telephone, '0', 0, 2);
    }
    // 310612345678 => 0649492174
    if (
      strlen($telephone) === 12 &&
      strpos($telephone, '310') === 0
    ) {
      $telephone = substr_replace($telephone, '0', 0, 3);
    }
    // 0031612345678 => 0649492174
    if (
      strlen($telephone) === 13 &&
      strpos($telephone, '0031') === 0
    ) {
      $telephone = substr_replace($telephone, '0', 0, 4);
    }
    // 00310612345678 => 0649492174
    if (
      strlen($telephone) === 14 &&
      strpos($telephone, '00310') === 0
    ) {
      $telephone = substr_replace($telephone, '0', 0, 5);
    }
  }
  return $telephone;
}

/**
 * Check if phone is valid.
 */
function procpush_valid_mobile_phone($mobile_phone) {
  $mobile_phone = procpush_valid_telephone($mobile_phone);
  return $mobile_phone;
}

/**
 * Check if phone is valid.
 */
function procpush_valid_fixed_phone($fixed_phone) {
  $fixed_phone = procpush_valid_telephone($fixed_phone);
  return $fixed_phone;
}

/**
 * Check if street is valid.
 */
function procpush_valid_street($street) {
  $street = ucfirst($street);
  return $street;
}

/**
 * Check if house number is valid.
 */
function procpush_valid_house_number($house_number) {
  $house_number = preg_replace('@[^0-9]@', '', $house_number);
  $regex = "@^[0-9]{1,5}([^0-9].*)?$@";
  if (!preg_match($regex, $house_number)) {
    $house_number = '';
  }
  return $house_number;
}

/**
 * Check if house number addition is valid.
 */
function procpush_valid_house_number_addition($house_number_addition) {
  if (!strlen($house_number_addition) > 0 || strlen($house_number_addition) > 16) {
    $house_number_addition = '';
  }
  return $house_number_addition;
}

/**
 * Check if postal code is valid.
 */
function procpush_valid_postal_code($postal_code) {
  if (preg_match('/^[1-9][0-9]{3} ?[a-zA-Z]{2}$/', $postal_code)) {
    // Nederlandse postcode.
    $postal_code = strtoupper($postal_code);
    $postal_code = str_replace(' ', '', $postal_code);
  }
  return $postal_code;
}

/**
 * Check if locality is valid.
 */
function procpush_valid_locality($locality) {
  $locality = strtoupper($locality);
  return $locality;
}

/**
 * Check if selections is valid.
 */
function procpush_valid_selections($selections) {
  if (!is_array($selections)) {
    $selections = array();
  }
  else {
    foreach (array('add', 'remove') as $type) {
      if (!empty($selections[$type])) {
        if (!is_array($selections[$type])) {
          unset($selections[$type]);
        }
      }
    }
    if (empty($selections['add']) && empty($selections['remove'])) {
      $selections = array();
    }
  }
  return $selections;
}

/**
 * Check if sp_active is valid.
 */
function procpush_valid_sp_active($sp_active) {
  if (!is_array($sp_active)) {
    $selections = array();
  }
  else {
    if (empty($sp_active['active']) && empty($sp_active['activities'])) {
      $sp_active = array();
    }
  }
  return $sp_active;
}

/**
 * Check if sp_work_and_int is valid.
 */
function procpush_valid_sp_work_and_int($sp_work_and_int) {
  if (!is_array($sp_work_and_int)) {
    $sp_work_and_int = array();
  }
  else {
    if (
      empty($sp_work_and_int['main_task']) &&
      empty($sp_work_and_int['industrial_sector']) &&
      empty($sp_work_and_int['occupational_group'])
    ) {
      $sp_work_and_int = array();
    }
  }
  return $sp_work_and_int;
}

/**
 * Check if overwrite is valid.
 */
function procpush_valid_overwrite($overwrite) {
  $overwrite = empty($overwrite) ? FALSE : TRUE;
  return $overwrite;
}

/**
 * Get realtion id from contact number.
 */
function procpush_get_relation_id($contact_number) {
  // Get relation id.
  $relation_id = db_select('procpush_id_map', 'map')
    ->fields('map', array('relation_id'))
    ->condition('map.contact_number', $contact_number)
    ->execute()
    ->fetchField();
  if (!empty($relation_id)) {
    return $relation_id;
  }
  return FALSE;
}
