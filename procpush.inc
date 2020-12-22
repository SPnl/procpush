<?php

/**
 * Checks wether a submission contains sufficient data to create new or sync existing civi contact.
 */
function procpush_check_submission_sufficient($data) {
  // Check if values are sufficient to create new or sync existing civi contact.
  if (!empty($data['relation_id'])) {
    return TRUE;
  }

  if (!empty($data['last_name']) || !empty($data['first_name'])) {
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
    if (!empty($data[$webformsync_key])) {
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
        $object['class_1_ff_344583'] = $data['sp_active']['active'];
      }
    }
    // Activiteiten SP.
    if (!empty($data['sp_active']['activities'])) {
      foreach ($data['sp_active']['activities'] as $activity) {
        if (isset($api_properties['class_1_ff_344584']['properties'][$activity])) {
          $object['class_1_ff_344584'][$activity] = TRUE;
        }
      }
    }
  }
  
  // SP Work and Interests field mapping.
  if (!empty($data['sp_work_and_int'])) {
    // Hoofdtaak.
    if (!empty($data['sp_work_and_int']['main_task'])) {
      foreach ($data['sp_work_and_int']['main_task'] as $task) {
        if (isset($api_properties['class_1_ff_344585']['properties'][$task])) {
          $object['class_1_ff_344585'][$task] = TRUE;
        }
      }
    }
    // Bedrijfstak.
    if (!empty($data['sp_work_and_int']['industrial_sector'])) {
      $sector = str_replace(', ', '_', $data['sp_work_and_int']['industrial_sector']);
      if (isset($api_properties['class_1_ff_344587']['properties'][$sector])) {
        $object['class_1_ff_344587'][$sector] = TRUE;
      }
    }
    // Beroepsgroep.
    if (!empty($data['sp_work_and_int']['occupational_group'])) {
      if (isset($api_properties['class_1_ff_344586']['properties'][$data['sp_work_and_int']['occupational_group']])) {
        $object['class_1_ff_344586'][$data['sp_work_and_int']['occupational_group']] = TRUE;
      }
    }
  }

  // Privacy.
  if (!empty($data['privacy'])) {
    foreach ($data['privacy'] as $privacy_option) {
      if (isset($api_properties['class_1_ff_123354']['properties'][$privacy_option])) {
        $object['class_1_ff_123354'][$privacy_option] = TRUE;
      }
    }
  }

  // Overwrite.
  if (isset($data['overwrite'])) {
    $object['class_2479_overwrite'] = empty($data['overwrite']) ? FALSE : TRUE;
  }

  if (procpush_check_address_sufficient($data)) {
    $object['class_1_relation_address_0']['country'] = 'NL';
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
  // Remove emojis.
  procpush_basic_cleanup($data);

  // Split name in first and last name when possible.
  if (!empty($data['name']) && empty($data['last_name'])) {
    $split_name = procpush_split_name($data['name']);    
    if (!empty($split_name['last_name'])) {
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
      $data['house_number_addition'] = empty($split_house_number['house_number_addition']) ? '' : $split_house_number['house_number_addition'];
    }
  }
  unset($data['house_number_and_addition']);

  // Convert phone to fixed_phone or mobile_phone.
  if (!empty($data['phone']) && empty($data['fixed_phone'])) {
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
    $query = "SELECT relation_id FROM {procpush_id_map} WHERE contact_number = :contact_number";
    $result = db_query($query, array('contact_number' => $data['contact_number']));
    if ($result) {
      $relation_id = $result->fetchField();
      if (!empty($relation_id)) {
        $data['relation_id'] = $relation_id;
      }
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
  $matches = array();
  $name = trim($name);
  $name = trim($name, '.,');

  // Remove comma from name.
  $comma_pos = strpos($name, ',');
  if ($comma_pos !== FALSE) {
    $first_half = trim(substr($name, 0, $comma_pos));
    $last_half = trim(substr($name, $comma_pos + 1));
    $name = $last_half . ' ' . $first_half;
  }

  // Replace terms.
  $replace = array(
    'v' => 'van',
    'v.' => 'van',
    'vd' => 'van de',
    'vd.' => 'van de',
    'v.d.' => 'van de',
    'v/d' => 'van de',
  );
  foreach ($replace as $orig => $new) {
    if (strpos($name, ' ' . $orig . ' ') !== FALSE) {
      $name = str_replace($orig, $new, $name);
    }
  }

  // Regexes for title and middle names.
  $titles_regex = 'Mr|mr|Mevr|mevr|Dhr|dhr|Ir|ir|Ing|ing|Bc|bc|Ms|ms|Drs|drs|Dr|dr';

  $middle_names_array = procpush_middle_names();
  $middle_names_regex = implode('|', $middle_names_array);

  // Split name on middle_name.
  $regex = "@^(?'title'(\b(" . $titles_regex . ")\b\.?\s)?)(?'first_name'(([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-\.]+)\s*)+?)(?'middle_name'(\b(" . $middle_names_regex . ")\b\s*))(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][a-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+)$@";
  $result = preg_match($regex, $name, $matches);
  if (!preg_match($regex, $name, $matches)) { 
    // Split name on middle name, last name first.
    $regex = "@^(?'title'(\b(" . $titles_regex . ")\b\.?\s)?)(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+),?\s(?'first_name'(([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-\.]+)\s*)+?)(?'middle_name'(\b(" . $middle_names_regex . ")\b\s*))$@";
    $result = preg_match($regex, $name, $matches);
    if (!preg_match($regex, $name, $matches)) { 
      // Split name without middle_name in first name and last name(s).
      $regex = "@^(?'title'(\b(" . $titles_regex . ")\b\.?\s)?)(?'first_name'(([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-\.]+)\s*)+?)(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+)$@";
      $result = preg_match($regex, $name, $matches);
      if (!preg_match($regex, $name, $matches)) { 
        // Split name in first name and last name, last name first.
        $regex = "@^(?'title'(\b(" . $titles_regex . ")\b\.?\s)?)(?'last_name'(\b[A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+\s?)+),?\s+(?'first_name'((([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ']\.)+)\s*)+?)$@";
        $result = preg_match($regex, $name, $matches);
        if (!preg_match($regex, $name, $matches)) { 
          // Check if only first name.
          $regex = "@^(?'first_name'([A-ZÀÈÌÒÙÁÉÍÓÚÝÂÊÎÔÛÃÑÕÄËÏÖÜŸÇÅa-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'][a-zàèìòùáéíóúýâêîôûãñõäëïöü¡¿çãñõ'\-]+))$@";
          $result = preg_match($regex, $name, $matches);
          if (!preg_match($regex, $name, $matches)) {
            // No match found.
            return FALSE;
          }
        }
      }
    }
  }
  // Set first character uppercase.
  $parts = array(
    'first_name',
    'last_name',
  );
  foreach ($parts as $part) {   
    if (isset($matches[$part])) {  
      $matches[$part] = trim($matches[$part]);
      if (!empty($matches[$part])) {
        $matches[$part] = ucfirst($matches[$part]);
      }
    }
  }
  if (empty($matches['last_name']) && !empty($matches['first_name'])) {
    $matches['last_name'] = $matches['first_name'];
    $matches['first_name'] = '';
   }
  return $matches;
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
    $split_house_number['house_number'] = $matches[1];
    $split_house_number['house_number_addition'] = $matches[2];
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
    if (isset($data[$key])) {
      if (!empty($data[$key])) {
        // Validate data with special function.
        if (function_exists('procpush_valid_' . $key)) {
          call_user_func('procpush_valid_' . $key, $data);
        }
      }
      // Remove empty data keys.
      if (empty($data[$key])) {
        unset($data[$key]);
      }
    }
  }
}

/**
 * Check if relation id is valid.
 */
function procpush_valid_relation_id(&$data) {
  $data['relation_id'] = preg_replace('/[^0-9]/s', '', $data['relation_id']);
  if (!empty($data['relation_id'])) {
    if (!is_numeric($data['relation_id']) || intval($data['relation_id']) != $data['relation_id'] || $data['relation_id'] <= 0) {
      $data['relation_id'] = '';
    }
  }
}

/**
 * Check if first name is valid.
 */
function procpush_valid_first_name(&$data) {
  $data['first_name'] = ucwords(strtolower($data['first_name']));
}

/**
 * Check if middle name is valid.
 */
function procpush_valid_middle_name(&$data) {
  $data['middle_name'] = strtolower($data['middle_name']);
}

/**
 * Check if last name is valid.
 */
function procpush_valid_last_name(&$data) {
  $data['last_name'] = ucwords(strtolower($data['last_name']));
}

/**
 * Check if email is valid.
 */
function procpush_valid_email(&$data) {
  if (!valid_email_address($data['email'])) {
    $data['email'] = '';
  }
  $data['email'] = strtolower($data['email']);
}

/**
 * Check if phone is valid.
 */
function procpush_valid_telephone(&$telephone) {
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
function procpush_valid_mobile_phone(&$data) {
  $data['mobile_phone'] = procpush_valid_telephone($data['mobile_phone']);
}

/**
 * Check if phone is valid.
 */
function procpush_valid_fixed_phone(&$data) {
  $data['fixed_phone'] = procpush_valid_telephone($data['fixed_phone']);
}

/**
 * Check if street is valid.
 */
function procpush_valid_street(&$data) {
  $data['street'] = ucfirst($data['street']);
}

/**
 * Check if house number is valid.
 */
function procpush_valid_house_number(&$data) {
  $data['house_number'] = preg_replace('@[^0-9]@', '', $data['house_number']);
  $regex = "@^[0-9]{1,5}([^0-9].*)?$@";
  if (!preg_match($regex, $data['house_number'])) {
    $data['house_number'] = '';
  }
}

/**
 * Check if house number addition is valid.
 */
function procpush_valid_house_number_addition(&$data) {
  if (!strlen($data['house_number_addition']) > 0 || strlen($data['house_number_addition']) > 16) {
    $data['house_number_addition'] = '';
  }
}

/**
 * Check if postal code is valid.
 */
function procpush_valid_postal_code(&$data) {
  if (preg_match('/^[1-9][0-9]{3} ?[a-zA-Z]{2}$/', $data['postal_code'])) {
    // Nederlandse postcode.
    $data['postal_code'] = strtoupper($data['postal_code']);
    $data['postal_code'] = str_replace(' ', '', $data['postal_code']);
  }
}

/**
 * Check if locality is valid.
 */
function procpush_valid_locality(&$data) {
  $data['locality'] = strtoupper($data['locality']);
}

/**
 * Check if selections is valid.
 */
function procpush_valid_selections(&$data) {
  if (!is_array($data['selections'])) {
    $data['selections'] = array();
  }
  else {
    foreach (array('add', 'remove') as $type) {
      if (!empty($data['selections'][$type])) {
        if (!is_array($data['selections'][$type])) {
          unset($data['selections'][$type]);
        }
      }
    }
    if (empty($data['selections']['add']) && empty($data['selections']['remove'])) {
      $data['selections'] = array();
    }
  }
}

/**
 * Check if sp_active is valid.
 */
function procpush_valid_sp_active(&$data) {
  if (!is_array($data['sp_active'])) {
    $selections = array();
  }
  else {
    if (empty($data['sp_active']['active']) && empty($data['sp_active']['activities'])) {
      $data['sp_active'] = array();
    }
  }
}

/**
 * Check if sp_work_and_int is valid.
 */
function procpush_valid_sp_work_and_int(&$data) {
  if (!is_array($data['sp_work_and_int'])) {
    $data['sp_work_and_int'] = array();
  }
  else {
    if (
      empty($data['sp_work_and_int']['main_task']) &&
      empty($data['sp_work_and_int']['industrial_sector']) &&
      empty($data['sp_work_and_int']['occupational_group'])
    ) {
      $data['sp_work_and_int'] = array();
    }
  }
}

/**
 * Check if overwrite is valid.
 */
function procpush_valid_overwrite($data) {
  $data['overwrite'] = empty($data['overwrite']) ? FALSE : TRUE;
}
