<?php

/**
 * Settings form.
 */
function procpush_test_form($form, &$form_state) {
  $storage = isset($form_state['storage']) ? $form_state['storage'] : array();

  $form = array();

  $form['text'] = array(
    '#markup' => '<p>Testing...</p>',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Get token',
  );
  return $form;
}

/**
 * Form submit function.
 */
function procpush_test_form_submit($form, &$form_state) {
  $storage = &$form_state['storage'];

  /*
  $token = procpush_get_oauth2_client_token();
  dpm($token);
  $registration_set = procpush_get_proc_registration_set();
  dpm($registration_set);
  //$schema = procpush_get_proc_form_schema(13);
  //dpm($schema);
  
  $data = array(
    'name' => 'Pietje Puk',
    'email' => 'pietjepuk@gmail.com',
    'street' => 'Camera Obscurastraat',
    'house_number' => '101',
    'postal_code' => '3813PK',
  );
  dpm('Creating object');
  $object = procpush_create_data_object($data);
  dpm($object);
  dpm('Validating object');
  $result = procpush_proc_validate_object($object, 13, $error);
  dpm('Result');
  dpm($result);
  dpm('Error');
  dpm($error);
  dpm('Pushing object');
  $result = procpush_proc_push_object($object, 13, $error);
  dpm('Result');
  dpm($result);
  dpm('Error');
  dpm($error);
  */

  $form_state['rebuild'] = TRUE;
}

