<?php

/**
 * Settings form.
 */
function procpush_settings_form($form, &$form_state) {
  $values = isset($form_state['values']) ? $form_state['values'] : array();
  $storage = &$form_state['storage'];

  $registration_form_id = variable_get('procapi_registration_form');

  $form['#prefix'] = '<div id="procpush_ajax_form">';
  $form['#suffix'] = '</div>';

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
  else {
    $form['feedback'] = array(
      '#markup' => 'Kies het registratie formulier in de instellingen de Procurios API module.',
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Opslaan',
    '#name' => 'submit',
  );

  return $form;
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
