<?php
/**
 * @file
 * Administrative page callbacks for the Google A/B Testing Experiments module.
 */

/**
 * Implements hook_admin_settings().
 */
function google_abtesting_admin_settings_form($form, &$form_state) {

  $form['google_abtesting_ga_domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Domain Name'),
    '#default_value' => variable_get('google_abtesting_ga_domain', ''),
    '#description' => t('Domain name declared as per the Google Analytic tracking code. This will used for Google Experiment "_udn" value.'),
  );

  return system_settings_form($form);
}

/**
 * Implements hook_form().
 *
 * Form to add URL's on which experiments need to implement.
 */
function google_abtesting_admin_add_experiments_form($form, &$form_state, $argument = '') {
  $exp_id = !empty($argument) ? check_plain($argument) : '';
  $form_values = $form_state['values'];
  $exp_path = !empty($form_values['experiment_url']) ? $form_values['experiment_url'] : '';
  $exp_language = !empty($form_values['experiment_language']) ? $form_values['experiment_language'] : '';
  $exp_key = !empty($form_values['experiment_key']) ? $form_values['experiment_key'] : '';

  if (!empty($exp_id) && $exp_id > 0) {
    $result = db_query('SELECT * FROM {google_abtesting} WHERE eid = :eid', array(':eid' => $exp_id));
    $record = $result->fetchAssoc();
    $exp_path = $record['path'];
    $exp_language = $record['language'];
    $exp_key = $record['experiment_key'];
  }

  $form['experiment_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Experiment Path'),
    '#default_value' => $exp_path,
    '#description' => t('Page URL in which you want to insert experiment code.'),
  );

  $form['experiment_language'] = array(
    '#type' => 'select',
    '#title' => t('Language'),
    '#options' => google_abtesting_get_language_options(),
    '#default_value' => $exp_language,
  );

  $form['experiment_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Experiment Key'),
    '#default_value' => $exp_key,
    '#description' => t("Experiment Key from Google Analytics. e.g '63724520-14'"),
  );

  $form['experiment_id'] = array(
    '#type' => 'hidden',
    '#value' => $exp_id,
  );

  $form['submit'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save'),
    ),
  );

  $form['submit'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save'),
    ),
  );
  return $form;
}

/**
 * Implements hook_form_validate().
 */
function google_abtesting_admin_add_experiments_form_validate($form, &$form_state) {
  if (empty($form_state['values']['experiment_url'])) {
    form_set_error('experiment_url', t('Please enter valid experiment path/URL'));
  }
  if (empty($form_state['values']['experiment_language'])) {
    form_set_error('experiment_language', t('Please enter select language'));
  }
  if (empty($form_state['values']['experiment_key'])) {
    form_set_error('experiment_key', t('Please enter experiment key provided from Google Analytics'));
  }
  else {
    if (preg_match('/[^\-0-9]/i', $form_state['values']['experiment_key'])) {
      form_set_error('experiment_key', t('Please enter a valid experiment key provided from Google Analytics'));
    }
  }
}

/**
 * Implements hook_form_submit().
 *
 * @see google_abtesting_admin_add_experiments_form()
 * @see google_abtesting_admin_add_experiments_form_validate()
 */
function google_abtesting_admin_add_experiments_form_submit($form, &$form_state) {
  $experiment_id = $form_state["values"]["experiment_id"];
  $path = $form_state["values"]["experiment_url"];
  $exp_key = trim(rtrim(ltrim($form_state["values"]["experiment_key"])));
  $language = $form_state["values"]["experiment_language"];

  // Insert the validated experiment in database or update the existing record.
  if (empty($experiment_id)) {
    // Add new record of experiment.
    db_insert('google_abtesting')
     ->fields(array(
       'path' => $path,
       'experiment_key' => $exp_key,
       'language' => $language,
     ))
    ->execute();
    $message = t('Added experiment successfully');
  }
  else {
    // Update the existing record.
    db_update('google_abtesting')
      ->fields(array(
        'path' => $path,
        'language' => $language,
        'experiment_key' => $exp_key,
      ))
      ->condition('eid', $experiment_id, '=')
      ->execute();

    $message = t('Updated experiment record successfully');
  }
  drupal_set_message($message);
  drupal_goto('admin/config/system/gab-experiments/list');
}

/**
 * Experiment delete confirmation form.
 *
 * @see google_abtesting_admin_experiments_list_form()
 */
function google_abtesting_admin_experiments_delete_confirm($form, &$form_state, $argument = '') {
  $form['experiments'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  $exp_id = !empty($argument) ? check_plain($argument) : '';
  $exp_path = db_query('SELECT path FROM {google_abtesting} WHERE eid = :eid', array(':eid' => $exp_id))->fetchField();
  // Current record to delete keep for reference.
  $form['experiments']['experiment_id'] = array(
    '#type' => 'hidden',
    '#value' => $exp_id,
    '#prefix' => '<li>',
    '#suffix' => check_plain($exp_path) . "</li>\n",
  );

  $form['operation'] = array('#type' => 'hidden', '#value' => 'delete');
  $form['#submit'][] = 'google_abtesting_admin_experiments_delete_confirm_submit';
  $confirm_question = t('Are you sure you want to delete this item?');
  return confirm_form($form,
    $confirm_question,
    'admin/config/system/gab-experiments/list', t('This action cannot be undone.'),
    t('Delete'), t('Cancel'));
}

/**
 * Form submission handler.
 *
 * @see google_abtesting_admin_experiments_list_form()
 */
function google_abtesting_admin_experiments_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $exp_id = $form_state['values']['experiments']['experiment_id'];
    $transaction = db_transaction();
    if (!empty($exp_id)) {
      try {
        // Delete record from DB.
        db_delete('google_abtesting')
          ->condition('eid', $exp_id, '=')
          ->execute();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('google_abtesting', $e);
        throw $e;
      }
    }
    watchdog('google_abtesting', 'Deleted experiment record for id - @eid.', array('@eid' => $exp_id));
    drupal_set_message(t('Deleted experiment successfully.'));
  }
  $form_state['redirect'] = 'admin/config/system/gab-experiments/list';
}

/**
 * Helper function to retrieve and return list of languages.
 *
 * @return array
 *   A keyed array of language options for use in a select form element.
 */
function google_abtesting_get_language_options() {
  $language_options = array();
  // language_list() returns a nested keyed array of enabled and disabled
  // languages with enabled languages at $all_languages[1].
  $all_languages = language_list('enabled');
  $enabled_languages = $all_languages[1];

  foreach (array_keys($enabled_languages) as $language_code) {
    $language_options[$language_code] = t('@language', array('@language', $enabled_languages[$language_code]->name));
  }
  return $language_options;
}
