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

  //g$token = procapi_get_oauth2_client_token();
  //gdpm($token);
  //gdpm('Get registration set.');
  //g$registration_set = procapi_registration_get_set();
  //gdpm($registration_set);
  //gdpm('Get schema.');
  //g$schema = procapi_registration_get_form_schema(12);
  //gdpm($schema);
  //g
  //g$data = array(
  //g  'name' => 'Pietje Puk',
  //g  'email' => 'pietjepuk@gmail.com',
  //g  'street' => 'Camera Obscurastraat',
  //g  'house_number' => '101',
  //g  'postal_code' => '3813PK',
  //g);
  //gdpm('Creating object');
  //g$object = procpush_create_data_object($data);
  //gdpm($object);
  //gdpm('Validating object');
  //g$result = procapi_registration_validate_object($object, 12, $error);
  //gdpm('Result');
  //gdpm($result);
  //gdpm('Error');
  //gdpm($error);
  //gdpm('Pushing object');
  //g$result = procapi_registration_push_object($object, 12, $error);
  //gdpm('Result');
  //gdpm($result);
  //gdpm('Error');
  //gdpm($error);

  $form_state['rebuild'] = TRUE;
}

