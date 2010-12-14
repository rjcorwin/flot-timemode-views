<?php

class flot_timemode_views_plugin_style extends views_plugin_style {
  function option_definition() {
    $options = parent::option_definition();
    $options['type'] = array('default' => 'line');
    $options['size'] = array('default' => '400x200');
    $options['x'] = array('default' => array('granularity' => 'auto', 'label' => 'default'));
    $options['y'] = array('default' => array('granularity' => 'auto', 'label' => 'default', 'pad' => 1));
    $options['layers'] = array('default' => array());
    return $options;
  }

  function options_form(&$form, &$form_state) {
    $form['type'] = array(
      '#type' => 'select',
      '#title' => t('Graph type'),
      '#options' => array('line' => t('Line'), 'bar' => t('Bar'), 'point' => t('Point')),
      '#description' => t("Choose the type of chart you would like to display."),
      '#default_value' => $this->options['type'],
    );

    $form['size'] = array(
      '#type' => 'textfield',
      '#title' => t('Size'),
      '#description' => t("Enter the dimensions for the chart. Format: WIDTHxHEIGHT (e.g. 200x100)"),
      '#default_value' => $this->options['size'],
    );

    // Generate label fields
    $label_options = array(
      '' => '< '. t('No labels') .' >',
      'default' => t('Default (from data points)'),
    );

    
    $form['x'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('X Axis'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['x']['label'] = array(
      '#type' => 'select',
      '#options'=> $label_options,
      '#title' => t('Labels'),
      '#default_value' => $this->options['x']['label'],
    );
    $form['y'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Y Axis'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['y']['label'] = array(
      '#type' => 'select',
      '#options'=> $label_options,
      '#title' => t('Labels'),
      '#default_value' => $this->options['y']['label'],
    );
    $form['y']['pad'] = array(
      '#type' => 'checkbox',
      '#title' => t('Add headroom above points'),
      '#default_value' => $this->options['y']['pad'],
    );
    $form['y']['custom-y-max'] = array(
      '#type' => 'textfield',
      '#title' => t('Y Max'),
      '#size' => 15,
      '#description' => t("(Optional) Enter the Y axis Max."),
      '#default_value' => $this->options['y']['custom-y-max'],
    );
    $form['y']['custom-y-min'] = array(
      '#type' => 'textfield',
      '#title' => t('Y Min'),
      '#size' => 15,
      '#description' => t("(Optional) Enter the Y axis Min."),
      '#default_value' => $this->options['y']['custom-y-min'],
    );

    $default_colors = array('#666', '#999', '#ccc');
    $layer_0 = (isset($this->options['colors']['1'])) ? $this->options['colors']['1'] : '#666';
    $layer_1 = (isset($this->options['colors']['2'])) ? $this->options['colors']['2'] : '#999';
    $layer_2 = (isset($this->options['colors']['3'])) ? $this->options['colors']['3'] : '#ccc';

    $form['colors'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Layer Colors'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['colors']['0'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 1'),
      '#size' => 15,
      '#default_value' => $layer_0,
    );
    $form['colors']['1'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 2'),
      '#size' => 15,
      '#default_value' => $layer_1,
    );
    $form['colors']['2'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 3'),
      '#size' => 15,
      '#default_value' => $layer_2,
    );

    // Views
    $layers = $this->get_views_by_style();
    unset($layers["{$this->view->name}:{$this->view->current_display}"]);
    $form['layers'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Additional data layers'),
      '#description' => t('Display the selected views displays as additional layers.'),
      '#options' => $layers,
      '#default_value' => $this->options['layers'],
    );
  }

  /**
   * Retrieve all views that have the specified style plugin.
   */
  protected function get_views_by_style($style_plugin = 'flot_timemode') {
    $views = views_get_all_views();
    $usable = array();
    foreach ($views as $view) {
      foreach (array_keys($view->display) as $display) {
        $view->set_display($display);
        if ($view->display_handler->get_option('style_plugin') === $style_plugin) {
          $usable["{$view->name}:{$display}"] = "{$view->name}:{$display}";
        }
      }
      $view->destroy();
    }
    ksort($usable);
    return $usable;
  }

  protected function build_layer($view, $result, $title = NULL) {
    // Get flot field, and bail if not present.
    if (isset($view->style_plugin) && method_exists($view->style_plugin, 'get_flot_field')) {
      $flot_field = $view->style_plugin->get_flot_field();
    }
    else {
      return;
    }

    $layer = array(
      'range' => array('min' => NULL, 'max' => NULL),
      'ticks' => array(),
      'series' => array(),
      'title' => $title,
    );

    // Iterate over results to build data and ticks
    foreach ($result as $id => $row) {
      $datapoint = $view->field[$flot_field]->flot_render($row);
      $value = $datapoint['value'];
      $label = isset($datapoint['label']) ? $datapoint['label'] : $datapoint['value'];
      
      $layer['series'][] = array($value[0], $value[1]);
      $layer['ticks'][$value[0]] = array($value[0], $label[0]);
      
      if (!isset($layer['range']['min']) || (!empty($value[1]) && $value[1] < $layer['range']['min'])) {
        $layer['range']['min'] = $value[1];
      }
      if (!isset($layer['range']['max']) || (!empty($value[1]) && $value[1] > $layer['range']['max'])) {
        $layer['range']['max'] = $value[1];
      }
    }
    $layer['series'] = new flotData($layer['series']);
    
    return $layer;
  }

  /**
   * Build each layer and retrieve its result set.
   */
  protected function get_layers() {
    $merged = array(
      'series' => array(),
      'titles' => array(),
      'ticks' => array(),
      'range' => array('min' => NULL, 'max' => NULL),
    );
    $layer_names["{$this->view->name}:{$this->view->current_display}"] = "{$this->view->name}:{$this->view->current_display}";
    $layer_names += array_filter($this->options['layers']);
    foreach ($layer_names as $layer_name) {
      list($view_name, $display_name) = explode(':', $layer_name);

      // Check that we're not going to build ourselves (and recurse to death).
      if ($layer_name === "{$this->view->name}:{$this->view->current_display}") {
        $layer = $this->build_layer($this->view, $this->view->result, $this->view->get_title());
      }
      // Otherwise, check that the view exists.
      else if ($view = views_get_view($view_name)) {
        $view->set_display($display_name);

        // If this display inherits arguments, pass the parent view's args.
        if ($view->display_handler->get_option('inherit_arguments')) {
          $view->set_arguments($this->view->args);
        }

        //Hack so all points will display.
        $view->pager['items_per_page'] = 0;
        $view->execute();
        $view->init_style();
        $layer = $this->build_layer($view, $view->result, $view->get_title());
      }

      if ($layer && !empty($layer['series']->data)) {
        // Merging series and titles is simple.
        $merged['series'][] = $layer['series'];
        $merged['titles'][] = $layer['title'];

        // Build an overlapping set of ticks between each series.
        foreach ($layer['ticks'] as $key => $val) {
          $merged['ticks'][$key] = $val;
        }

        // Choose the maximum of all series and minimum of all series for range.
        if (!isset($merged['range']['min']) || $layer['range']['min'] < $merged['range']['min']) {
          $merged['range']['min'] = $layer['range']['min'];
        }
        if (!isset($merged['range']['max']) || $layer['range']['max'] > $merged['range']['max']) {
          $merged['range']['max'] = $layer['range']['max'];
        }
      }
    }
    $merged['ticks'] = array_values($merged['ticks']);
    return $merged;
  }

  /**
   * Theme template preprocessor.
   */
  function preprocess(&$vars) {
    $view = $this->view;
    $options = $this->options;

    // Parameters
    $type = !empty($options['type']) ? $options['type'] : 'line';
    $size = !empty($options['size']) ? explode('x', $options['size']) : array('200','100');

    // DOM element options
    $element = array();
    $element['style'] = is_numeric($size[0]) ? "width:{$size[0]}px;" : "width:{$size[0]};";
    $element['style'] .= is_numeric($size[1]) ? "height:{$size[1]}px;" : "height:{$size[1]};";
    $vars['element'] = $element;

    // Build layers.
    $layers = $this->get_layers();

    $vars['data'] = $layers['series'];
    $vars['titles'] = $layers['titles'];
    $range = $layers['range'];
    $ticks = $layers['ticks'];

    // Set up the type class, set axes
    switch ($options['type']) {
      case 'point':
        $style = new flotStylePoint();
        break;
      case 'bar':
        $style = new flotStyleBar();
        break;
      case 'line':
      default:
        $style = new flotStyleLine();
        break;
    }

    // Format Y Axis
    $granularity = 0;

    // If max is too small Flot barfs -- set a minimum value
    $range['max'] = ($range['max'] < 5) ? 5 : $range['max'];

    // Pad Y axis if necessary
    if ($options['y']['pad']) {
      if ($range['min'] > 0) {
        $range['min'] = 0;
      }
      else {
        $range['min'] = ceil($range['min'] + ($range['min'] * .1));
      }
      $range['max'] = floor($range['max'] + ($range['max'] * .1));
    }

    // If a custom y-axis range is set, change to those values.
    if (!empty($this->options['y']['custom-y-max'])) {
      $range['max'] = $this->options['y']['custom-y-max'];
    }
    if (!empty($this->options['y']['custom-y-min'])) {
      $range['min'] = $this->options['y']['custom-y-min'];
    }

    if (!empty($options['y']['label'])) {
      switch ($options['y']['granularity']) {
        case 'endpoints':
          $yticks = array(array($range['min'], $range['min']), array($range['max'], $range['max']));
          $style->axis_ticks('yaxis', $yticks);
          break;
        case 'auto':
          $style->axis_range('yaxis', $range);
          break;
        default:
          $style->axis_range('yaxis', $range, $options['y']['granularity']);
          break;
      }
    }
    else {
      $style->yaxis->ticks = array();
    }

    if (count($ticks) > 1 && !empty($options['x']['label'])) {
      $domain = array(reset($ticks), end($ticks));
      switch ($options['x']['granularity']) {
        case 'endpoints':
          $style->axis_ticks('xaxis', $domain);
          break;
        case 'auto':
          $style->axis_ticks('xaxis', $ticks);
          break;
        default:
          $domain_range = array($domain[0][0], $domain[1][0]);
          $style->axis_range('xaxis', $domain_range, $options['x']['granularity']);
          break;
      }
    }
    else {
      $style->xaxis->ticks = array();
    }

    // Set colors.
    if (isset($options['colors']['0'])) {
        $style->colors[0] = $options['colors']['0'];
    }

    if (isset($options['colors']['1'])) {
        $style->colors[1] = $options['colors']['1'];
    }
    if (isset($options['colors']['2'])) {
        $style->colors[2] = $options['colors']['2'];
    }

    $vars['options'] = $style;
    $vars['options']->xaxis->mode = 'time';
    unset($vars['options']->xaxis->ticks);

  }

  /**
   * Validate function.
   */
  function validate() {
    parent::validate();
    $field = $this->get_flot_field();
    if (!$field) {
      return array(t('You must use a field that is compatible (e.g. <strong>Data point</strong>) with Flot to use the Flot style plugin.'));
    }
  }

  /**
   * Get the first usable flot field on this view.
   */
  function get_flot_field() {
    $fields = $this->display->handler->get_option('fields');
    foreach ($fields as $field => $info) {
      $handler = get_class(views_get_handler($info['table'], $info['field'], 'field'));
      if (method_exists($handler, 'flot_render')) {
        return $field;
      }
    }
    return FALSE;
  }
}