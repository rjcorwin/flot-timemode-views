<?php

/**
 * Implementation of hook_views_data_alter().
*/ 
function flot_timemode_views_data_alter(&$data) {
  $data['views']['timemode_datapoint'] = array(
    'title' => t('Timemode Data point'),
    'help' => t('Provide a datapoint suitable for a flot graph.'),
    'field' => array(
      'handler' => 'flot_timemode_handler_field_datapoint',
    ),
  );
}

/**
 * Implementation of hook_views_handlers().
*/
function flot_timemode_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'flot_timemode') . '/views',
    ),
    'handlers' => array(
      'flot_timemode_handler_field_datapoint' => array('parent' => 'views_handler_field'),
    ),
  );
}

/**
 * Implementation of hook_views_plugins().
 */
function flot_timemode_views_plugins() {
$i = 'i';
  return array(
    'style' => array(
      'flot_timemode' => array(
        'title' => t('Flot Timemode'),
        'help' => t('Displays the result set as a flot graph.'),
        'handler' => 'flot_timemode_views_plugin_style',
        'theme' => 'flot_timemode_views_style',
        'theme path' => drupal_get_path('module', 'flot_timemode') .'/views',
        'path' => drupal_get_path('module', 'flot_timemode') .'/views',
        'even empty' => TRUE,
        'uses row plugin' => FALSE,
        'uses fields' => TRUE,
        'uses options' => TRUE,
        'type' => 'normal',
      ),
    ),
  );
}

/**
 * Preprocessor for theme_flot_views_style().
 */
function template_preprocess_flot_timemode_views_style(&$vars) {
  $options = $vars['view']->style_plugin->preprocess($vars);
}
