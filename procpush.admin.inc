<?php

/**
 * Settings form.
 */
function procpush_settings_form($form, &$form_state) {
  $values = isset($form_state['values']) ? $form_state['values'] : array();
  $storage = &$form_state['storage'];


  $form['#prefix'] = '<div id="procpush_ajax_form">';
  $form['#suffix'] = '</div>';

  // Get target forms from procurios.
  $api_forms_list = procapi_get_proc_forms_list();
  $form['procpush_api_form'] = array(
    '#type' => 'select',
    '#title' => 'Kies het procurios API formulier',
    '#name' => 'procpush_api_form',
    '#options' => $api_forms_list,
    '#default_value' => variable_get('procpush_api_form'),
    '#required' => TRUE,
    '#ajax' => array(
      'wrapper' => 'procpush_ajax_form',
      'callback' => 'procpush_ajax_form',
    ),
  );

  $form_id = variable_get('procpush_api_form', '');
  if (!empty($form_id)) {
    $form['field_mapping'] = array(
      '#type' => 'fieldset',
      '#title' => 'Veld mapping',
    );
    $webformsync_fields = spwebformsync_contact_data_fields();
    $procurios_fields = procapi_get_proc_form_schema_contact_fields($form_id);
    foreach ($webformsync_fields as $webformsync_field_key => $webformsync_field_title) {
      $form['field_mapping']['procpush_field_' . $webformsync_field_key . '_mapping'] = array(
        '#type' => 'select',
        '#title' => $webformsync_field_title,
        '#options' => $procurios_fields,
        '#empty_option' => '- selecteer -',
        '#default_value' => variable_get('procpush_field_' . $webformsync_field_key . '_mapping', ''),
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
    'procpush_api_form',
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
