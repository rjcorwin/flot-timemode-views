<?php

/**
 * @file flot-views-summary-style.tpl.php
 * Template to display a flot summary view.
 *
 * - $element : An array representation of the flot DOM element.
 * - $data: A flotData object.
 * - $options: A flotStyle object.
 */
?>

<div class="views-flot">
  <?php print theme('flot_timemode_graph', $element, $data, $options); ?>
</div>
