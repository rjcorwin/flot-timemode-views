<?php

class flot_timemode_handler_field_datapoint extends views_handler_field  {
  protected $use_date;
  /**
   * Disable advanced render.
   */
  function allow_advanced_render() { return FALSE; }

  /**
   * Option defaults.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['series'] = array('default' => array('field' => ''));
    $options['value'] = array('default' => array('field' => ''));
    return $options;
  }

  /**
   * Options form.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['series'] = array(
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Series (X axis)'),
    );
    $form['series']['field'] = array(
      '#title' => t('Field'),
      '#type' => 'select',
      '#options' => array(),
      '#default_value' => $this->options['series']['field'],
    );

    $form['value'] = array(
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Series (Y axis)'),
    );
    $form['value']['field'] = array(
      '#title' => t('Field'),
      '#type' => 'select',
      '#options' => array(),
      '#default_value' => $this->options['value']['field'],
    );

    // Get field options and generate subsequent options based on class
    foreach (views_fetch_fields($this->view->base_table, 'field') as $field_id => $field_info) {
      $form['series']['field']['#options'][$field_id] = $field_id;
      $form['value']['field']['#options'][$field_id] = $field_id;

      list($table, $field) = explode('.', $field_id);
      $handler = get_class(views_get_handler($table, $field, 'field'));
      if (strpos($handler, '_date') !== FALSE) {
        // If the series field is empty, use a date by default
        $form['series']['field']['#default_value'] = empty($form['series']['field']['#default_value']) ? $field_id : $form['series']['field']['#default_value'];
        // Add this field to the list of timestamp options which trigger the grouping field display.
        $form['series']['grouping']['#dependency']['edit-options-series-field'][] = $field_id;
        $form['series']['date_format']['#dependency']['edit-options-series-field'][] = $field_id;
      }
    }

  }

  /**
   * Query method.
   */
  function query() {
    // Killswitch
    if (empty($this->options['series']['field']) || empty($this->options['value']['field'])) {
      return;
    }

    // Yank off the base field & any other groupby's / having's. They will screw up our results.
    unset($this->query->fields[$this->view->base_field]);
    $this->query->groupby = array();
    $this->query->having = array();

    // Add in series field
    list($series_table, $series_field) = explode('.', $this->options['series']['field']);
    $alias = $this->query->ensure_table($series_table);

    // Determine whether we are using a date field.
    $handler = get_class(views_get_handler($series_table, $series_field, 'field'));
    $this->use_date = strpos($handler, '_date') !== FALSE;

    $field_alias = $this->query->add_field($series_table, $series_field);
    $this->query->add_groupby($field_alias);
    $this->series_table = $alias;
    $this->series_field = $field_alias;

    // Add in value field
    list($value_table, $value_field) = explode('.', $this->options['value']['field']);
    $alias = $this->query->ensure_table($value_table);

    if ($this->use_date) {
      $field_alias = $this->query->add_field($value_table, $value_field);
    }
    else {
      switch ($this->options['value']['format']) {
        case 'count':
          $field_alias = $this->query->add_field(NULL, "COUNT({$alias}.{$value_field})", "{$value_table}_{$value_field}", array('aggregate' => TRUE));
          break;
        default:
          $field_alias = $this->query->add_field($value_table, $value_field);
          break;
      }
    }
    $this->value_table = $alias;
    $this->value_field = $field_alias;
    $this->query->set_count_field($alias, $value_field, $field_alias);
  }

  /**
   * Return an array usable as a plot point by flot.
   */
  function flot_render($values) {
    if ($this->use_date) {
      $series = $values->{$this->series_field};
      $series_label = format_date($values->{$this->series_field}, $this->options['series']['date_format']);
    }
    else {
      $series_label =
      $series = check_plain($values->{$this->series_field});
    }
    $value = check_plain($values->{$this->value_field});
    return array(
      'value' => array($series, $value),
      'label' => array($series_label, $value),
    );
  
  }

  /**
   * Optional method that allows the data source to determine axis bounds.
   */
  function pre_render($result) {
    foreach($result as $row) {
      //
      $original_date = new DateTime(NULL, new DateTimeZone(date_default_timezone_name()));
      $original_date->setTimestamp($row->node_created);
      $adjusted_date = new DateTime($original_date->format('Y-m-d H:i:s'), new DateTimeZone('UTC'));
      $row->node_created = $adjusted_date->format('U');
      // Convert the Unix timestamp to milliseconds so Javascript can understand it
      $row->node_created = $row->node_created * 1000;
      $rows[] = $row;
    }
    $this->view->result = $rows;
  }

  /**
   * The default render function.
   */
  function render($values) {
    $series = check_plain($values->{$this->series_field});
    $value = check_plain($values->{$this->value_field});
    return "$series, $value";
  }
}
