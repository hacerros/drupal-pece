<?php

/**
 * Parse a YAML stream.
 *
 * @param $input
 *   The YAML stream.
 *
 * @return mixed
 *   Array of parsed YAML elements (or FALSE on failure).
 */
function pece_parse_yaml($input = '') {
  if (function_exists('yaml_parse')) {
    return yaml_parse($input);
  }

  // Load fallback library.
  if ($path_to_spyc_library = libraries_get_path('spyc')) {
    include_once $path_to_spyc_library . '/Spyc.php';
  }

  if (!function_exists('spyc_load')) {
    drupal_set_message(t('Parse YAML: Unable to find a YAML parser.'), 'error', FALSE);
    return FALSE;
  }

  try {
    $parsed = spyc_load($input);
  } catch(Exception $exception) {
    drupal_set_message(t('Failed parsing YAML stream. ') . $exception->getMessage(), 'error');
    return FALSE;
  }
  return $parsed;
}

/**
 * Setup core permissions.
 *
 * @param
 *   A associative array map of permissions to grant/remove.
 *   Example:
 *     array(
 *       'all' => array(
 *         'use Search Autocomplete' => TRUE, // Sample for granting to all user roles.
 *       ),
 *       'role name' => array(
 *         'granting permission' => TRUE,  // Sample for granting a permission.
 *         'removing permission' => FALSE, // Sample for removing a permission.
 *       ),
 *     );
 */
function pece_set_permissions($permission_map = array()) {
  $all_permissions = user_permission_get_modules();
  $user_roles = user_roles();

  foreach ($user_roles as $rid => $role_name) {
    $assigning_permissions = empty($permission_map[$role_name]) ? array() : $permission_map[$role_name];
    $assigning_permissions += empty($permission_map['all']) ? array() : $permission_map['all'];

    // Clean-up non-existing permissions.
    foreach ($assigning_permissions as $permission_name => $change) {
      if (empty($all_permissions[$permission_name])) {
        unset($assigning_permissions[$permission_name]);
      }
    }

    user_role_change_permissions($rid, $assigning_permissions);
  }
}

/**
 * Helper to setup ECK entities default permissions.
 *
 * @param $eck_entity_name
 *  ECK Entity machine name
 *  Example:
 *     array(
 *       'all' => array(
 *         "eck view <eck_entity_name> <eck_entity_name> entities" => TRUE,
 *       ),
 *       'role name' => array(
 *          "eck add <eck_entity_name> <eck_entity_name> entities",
 *          "eck delete <eck_entity_name> <eck_entity_name> entities",
 *          "eck edit <eck_entity_name> <eck_entity_name> entities",
 *          "eck list <eck_entity_name> <eck_entity_name> entities",
 *       ),
 *     );
 *
 * @see pece_set_permissions().
 */
function _pece_eck_set_permissions($eck_entity_name) {

  $entity_permissions = array(
    'all' => array(
      "eck view $eck_entity_name $eck_entity_name entities" => TRUE
    )
  );

  $user_roles = array(
    'Contributor',
    'Researcher',
    'administrator',
    'editor',
  );

  $pece_entities_entity_perm = array(
    "eck add $eck_entity_name $eck_entity_name entities",
    "eck delete $eck_entity_name $eck_entity_name entities",
    "eck edit $eck_entity_name $eck_entity_name entities",
    "eck list $eck_entity_name $eck_entity_name entities",
  );

  foreach ($pece_entities_entity_perm as $name) {
    foreach ($user_roles as $role) {
      $entity_permissions[$role][$name] = TRUE;
    }
  }

  pece_set_permissions($entity_permissions);
  watchdog('info', 'ECK Entity (' . $eck_entity_name . ') permissions has been updated.');
}


/**
 * Helper method to reset an entity's panelizer state for a given view mode.
 */
function pece_reset_panelizer_entity_to_default($entity_type, $entity, $view_mode = NULL) {
  $handler = panelizer_entity_plugin_get_handler($entity_type);
  if (isset($entity->panelizer[$view_mode])) {
    $handler->delete_entity_panelizer($entity, $view_mode);
  }
}

/**
 * Helper method to reset many entity's panelizer state for a given view mode.
 */
function pece_reset_panelizer_entities_to_default($entity_type, $entities, $view_mode = NULL) {
  foreach ($entities as $entity) {
    pece_reset_panelizer_entity_to_default($entity_type, $entity, $view_mode);
  }
}

/**
 * Rebuild a given component.
 * @param $component
 *        The name of the component type.
 * @param $module
 *        The module to rebuild the component from. Optional. Default to all.
 */
function pece_defaultconfig_component_rebuild_by_name($component, $module = FALSE) {
  $components = defaultconfig_get_components();
  if (!empty($components[$component])) {
    defaultconfig_component_rebuild($components[$component], $module);
  }
}

/**
 * Retrieves the default value for a field.
 * @param $entity_type
 *        The entity type where the field is used.
 * @param $bundle
 *        The bundle where the field is used.
 * @param $field_name
 *        The name of the field to grab the default value from.
 * @param $language
 *        An optional language to grab the default value from.
 * @return array
 *   Field value structure as an array.
 */
function pece_get_default_field_value($entity_type, $bundle, $field_name, $language = NULL) {
  $entity_info = entity_get_info($entity_type);
  $bundle_key = !empty($entity_info['bundle keys']['bundle']) ? $entity_info['bundle keys']['bundle'] : 'type';
  $entity = entity_create($entity_type, array($bundle_key => $bundle));
  $field = field_info_field($field_name);
  $instance = field_info_instance($entity_type, $field_name, $bundle);
  return field_get_default_value($entity_type, $entity, $field, $instance, $language);
}

/**
 * Checks if a given/current user has adminsitrator role.
 *
 * @param $account
 *        User object
 * @return bool
 */
function _pece_is_admin($account = FALSE) {
  if (empty($account)) {
    global $user;
    $account = $user;
  }
  $admin_role = user_role_load_by_name('administrator');
  if ($account->uid == 1) return TRUE;
  if (user_has_role($admin_role->rid, $account)) return TRUE;

  return FALSE;
}

/**
 * Checks if a given/current user has Contributor role.
 */
function _pece_is_contributor($account) {
  if (empty($account)) {
    global $user;
    $account = $user;
  }
  $contributor_role = user_role_load_by_name('Contributor');
  return (in_array($contributor_role->rid, array_keys($account->roles)));
}

/**
 * Checks if a given/current user has Researcher role.
 */
function _pece_is_researcher($account) {
  if (empty($account)) {
    global $user;
    $account = $user;
  }
  $contributor_role = user_role_load_by_name('Researcher');
  return (in_array($contributor_role->rid, array_keys($account->roles)));
}

/**
 * Update Artifact's media fields settings (audio, image, pdf, video).
 */
function _pece_artifacts_media_field_settings_update() {
  $artifatct_fields = array(
    'pece_artifact_audio' => 'field_pece_media_audio',
    'pece_artifact_image' => 'field_pece_media_image',
    'pece_artifact_pdf' => 'field_pece_media_pdf',
    'pece_artifact_video' => 'field_pece_media_video',
  );
  foreach ($artifatct_fields as $node_type => $field_name) {
    _pece_artifacts_set_public_allowed_schema($node_type, $field_name);
  }
}

/**
 * Add Public to allowed_scheme on a given field of a node.
 *
 * @param $node_type
 * @param $field_name
 * @throws \FieldException
 */
function _pece_artifacts_set_public_allowed_schema($node_type, $field_name) {
  $field_instance_info = field_info_instance('node', $field_name, $node_type);
  $field_instance_info ['widget']['settings']['allowed_schemes']['public'] = 'public';
  field_update_instance($field_instance_info);
}

/**
 *
 */
function pece_batch_unassign_role($rolename) {
  $role = user_role_load_by_name($rolename);
  $users = pece_get_users_by_rolename($rolename);
  if (empty($users)) {
    return;
  }
  user_multiple_role_edit(array_keys($users['user']), 'remove_role', $role->rid);
}

function pece_get_users_by_rolename($rolename){
  $query = new EntityFieldQuery;
  $query->entityCondition('entity_type', 'user');
  $query->addTag('rolequery');
  $query->addMetaData('rolename', $rolename);
  return $query->execute();
}

/**
 * Implement hook_query_TAG_alter
 *
 * @param QueryAlterableInterface $query
 */
function pece_query_rolequery_alter(QueryAlterableInterface $query) {
  $rolename = $query->getMetaData('rolename');
  $role_subquery = db_select("role", "role");
  $role_subquery->condition('role.name', $rolename, '=');
  $role_subquery->join('users_roles', "users_to_include", "role.rid = users_to_include.rid");
  $role_subquery->fields('users_to_include', array('uid' => 'uid'));
  $role_subquery->where('users_to_include.uid = users.uid');
  $query->exists($role_subquery);
}
