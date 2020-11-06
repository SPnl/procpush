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


