<?php

/**
 * Checks wether a submission contains sufficient data to create new or sync existing civi contact.
 */
function procpush_check_submission_sufficient($data) {
  // Check if values are sufficient to create new or sync existing civi contact.
  if (!empty($data['contact_id'])) {
    return TRUE;
  }

  if (!empty($data['last_name'])) {
    if (!empty($data['email']) || !empty($data['telephone']) || procapi_check_address_sufficient($data)) {
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
  $form_id = variable_get('procpush_api_form');
  $api_schema_raw = procapi_registration_get_form_schema($form_id);
  $api_properties = $api_schema_raw[$form_id]['properties'];

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

function procpush_data_fields() {
  $data_names = array(
    'contact_id' => 'contact id',
    'name' => 'gehele naam',
    'first_name' => 'voornaam',
    'middle_name' => 'tussenvoegsel',
    'last_name' => 'achternaam',
    'email' => 'e-mail',
    'phone' => 'telefoon',
    'fixed_phone' => 'telefoon vast',
    'mobile_phone' => 'telefoon mobiel',
    'street' => 'straat',
    'house_number_and_addition' => 'huisnummer inclusief eventuele toevoeging',
    'house_number' => 'huisnummer',
    'house_number_addition' => 'huisnummer toevoeging',
    'postal_code' => 'postcode',
    'locality' => 'plaats',
  );
  return $data_names;
}
