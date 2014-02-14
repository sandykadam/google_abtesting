<?php
/**
 * @file
 * Google A/B Testing Content Experiments Lists.
 * Admin interface to display listing of experiments.
 */

/**
 * Form builder: Builds the Google experiment list administration overview.
 */
function google_abtesting_admin_experiments_list_form($form, &$form_state) {
  // Build the sortable table header.
  $header = array(
    'path' => array('data' => t('Path'), 'field' => 'ga.path'),
    'code' => array('data' => t('Experiement Code'), 'field' => 'ga.experiment_key'),
    'language' => array('data' => t('Language'), 'field' => 'ga.language'),
  );

  $header['operations'] = array('data' => t('Operations'));

  $query = db_select('google_abtesting', 'ga')->extend('PagerDefault')->extend('TableSort');

  $results = $query
    ->fields('ga')
    ->limit(30)
    ->orderByHeader($header)
    ->execute()
    ->fetchAll();

  // Prepare the list of records.
  $languages = language_list();
  $destination = drupal_get_destination();
  $options = array();
  foreach ($results as $row) {
    $langcode = $row->language;

    $options[$row->eid] = array(
      'path' => $row->path,
      'code' => $row->experiment_key,
      'language' => $langcode,
    );

    if ($langcode == LANGUAGE_NONE || isset($languages[$langcode])) {
      $options[$row->eid]['language'] = $langcode == LANGUAGE_NONE ? t('Language neutral') : t('@language', array('@language' => $languages[$langcode]->name));
    }
    else {
      $options[$row->eid]['language'] = t('Undefined language (@langcode)', array('@langcode' => $langcode));
    }

    // Build a list of all the accessible operations for the current record.
    $operations = array();
    $operations['edit'] = array(
      'title' => t('edit'),
      'href' => 'admin/config/system/gab-experiments/edit/' . $row->eid,
      'query' => $destination,
    );

    $operations['delete'] = array(
      'title' => t('delete'),
      'href' => 'admin/config/system/gab-experiments/delete/' . $row->eid,
      'query' => $destination,
    );

    $options[$row->eid]['operations'] = array();
    if (count($operations) > 1) {
      // Render an unordered list of operations links.
      $options[$row->eid]['operations'] = array(
        'data' => array(
          '#theme' => 'links__node_operations',
          '#links' => $operations,
          '#attributes' => array('class' => array('links', 'inline')),
        ),
      );
    }
    elseif (!empty($operations)) {
      // Render the first and only operation as a link.
      $link = reset($operations);
      $options[$row->eid]['operations'] = array(
        'data' => array(
          '#type' => 'link',
          '#title' => $link['title'],
          '#href' => $link['href'],
          '#options' => array('query' => $link['query']),
        ),
      );
    }
  }

  $form['experiment_list'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $options,
    '#empty' => t('No records available.'),
  );

  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}
