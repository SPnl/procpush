<?php

/**
 * Settings form.
 */
function procpush_settings_form($form, &$form_state) {
  $values = isset($form_state['values']) ? $form_state['values'] : array();
  $storage = &$form_state['storage'];

  $registration_form_fields = procapi_registration_get_fields();
  $profile_form_fields = procapi_profile_get_fields();

  if ($registration_form_fields !== $profile_form_fields) {
    drupal_set_message('The registration and profile api have different fields! This should be fixed in Procurios.', 'warning');
  }

  $form['#prefix'] = '<div id="procpush_ajax_form">';
  $form['#suffix'] = '</div>';

  // Get target forms from procurios.
  $api_registration_form_list = procapi_registration_get_forms();
  $form['procpush_api_registration_form'] = array(
    '#type' => 'select',
    '#title' => 'Kies het procurios API registratie formulier voor het inschieten van nieuwe contacten',
    '#name' => 'procpush_api_registration_form',
    '#options' => $api_registration_form_list,
    '#default_value' => $registration_form_id,
    '#required' => TRUE,
    '#ajax' => array(
      'wrapper' => 'procpush_ajax_form',
      'callback' => 'procpush_ajax_form',
    ),
  );

  // Get target forms from procurios.
  $api_profile_form_list = procapi_profile_get_forms();
  $form['procpush_api_profile_form'] = array(
    '#type' => 'select',
    '#title' => 'Kies het procurios API profiel formulier voor het updaten van bestaande contacten',
    '#name' => 'procpush_api_profile_form',
    '#options' => $api_profile_form_list,
    '#default_value' => $profile_form_id,
    '#required' => TRUE,
    '#ajax' => array(
      'wrapper' => 'procpush_ajax_form',
      'callback' => 'procpush_ajax_form',
    ),
  );

  if (!empty($registration_form_id)) {
    $form['field_mapping'] = array(
      '#type' => 'fieldset',
      '#title' => 'Veld mapping',
    );
    $procpush_fields = procpush_data_fields_flat_list();
    $procurios_fields = procapi_registration_get_fields();
    foreach ($procpush_fields as $procpush_field_key => $procpush_field_title) {
      $form['field_mapping']['procpush_field_' . $procpush_field_key . '_mapping'] = array(
        '#type' => 'select',
        '#title' => $procpush_field_title,
        '#options' => $procurios_fields,
        '#empty_option' => '- selecteer -',
        '#default_value' => variable_get('procpush_field_' . $procpush_field_key . '_mapping', ''),
      );
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Opslaan',
    '#name' => 'submit',
  );

  return $form;
}

/**
 * Validate settings form.
 */
function procpush_settings_form_validate($form, &$form_state) {
  $values = &$form_state['values'];
  $storage = &$form_state['storage'];

  // Store values.
  foreach (array(
    'procpush_api_registration_form',
    'procpush_api_profile_form',
  ) as $form_key) {
    if (!empty($values[$form_key])) {
      variable_set($form_key, $values[$form_key]);
    }
  }
}

/**
 * Submit settings form.
 */
function procpush_settings_form_submit($form, &$form_state) {
  $values = &$form_state['values'];
  $storage = &$form_state['storage'];

  $mapping = array();
  foreach ($values as $key => $value) {
    if (strpos($key, 'procpush_') !== FALSE) {
      preg_match('#^procpush_field_(.*)_mapping$#', $key, $matches);
      if (!empty($matches[1]) && !empty($value)) {
        $mapping[$matches[1]] = $value;
      }
      variable_set($key, $value);
    }
  }
  variable_set('procpush_field_mapping', $mapping);
}

/**
 * Ajax return form.
 */
function procpush_ajax_form($form, &$form_state) {
 return $form;
}
