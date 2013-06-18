<?php

/**
 * @file
 * UEditor Module for Drupal 7.x
 *
 */

/**
 * HOOK_ueditor_allowed_extensions
 *
 * e.g. $allowed_exts = array('zip','rar');
 *
 */
function hook_ueditor_allowed_extensions(&$allowed_exts) {
  $allowed_exts = array('zip','rar');
}

