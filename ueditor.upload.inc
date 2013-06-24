<?php

/**
 * @file
 * upload_class for UEditor
 *
 * created by M <55510696 AT qq.com>
 *
 */

final class ueditor_upload {
  private $_images  = array('jpg', 'jpeg', 'gif', 'png');
  private $_keyarr  = array('name','type','tmp_name','size');
  private $_configs = array();
  private $_results = array('original'=>'','name'=>'','url'=>'','size'=>'','type'=>'','state'=>'');
  public $settings = NULL;
  public function __construct($config=NULL) {
    $this->_configs = array(
      'webroot'      => '/',
      'saved_path'   => 'sites/default/files/',
      'file_handle'  => 'upfile',
      'file_type'    => 'image',
      'allowed_exts' => array(),
      'allowed_size' => 4 * 1024 * 1024,
    );
    $this->setConfig($config);
  }
  public function setConfig($key, $val=NULL) {
    if( is_array($key) )
      $this->_configs = $key + $this->_configs;
    elseif($key)
      $this->_configs[$key] = $val;
    return $this;
  }
  public function getConfig($key) {
    return isset($this->_configs[$key]) ? $this->_configs[$key] : NULL;
  }
  public function doUpload($base64=FALSE) {
    if(!$base64 and !isset($_FILES[ $this->_configs['file_handle'] ])) {
      $this->_results['state'] = 'unvalidate form file_handle';
      return $this;
    }
    if($base64) {
      $this->base64File();
      return $this;
    }
    $file = $_FILES[ $this->_configs['file_handle'] ];
    if( !$this->_is_legal($file) ) {
      $this->_results['state'] = 'unvalidate form data';
      return $this;
    }
    $filename = strtolower($file['name']);
    if($this->_configs['file_type']=='image' and !$this->_is_image($filename)) {
      $this->_results['state'] = 'not an image';
      return $this;
    }
    if($this->_configs['file_type']!='image' and !$this->_is_allowed_exts($filename) ) {
      $this->_results['state'] = 'not allowed filetype';
      return $this;
    }
    if( !$this->_is_allowed_size($file['size']) ) {
      $this->_results['state'] = 'filesize out of range';
      return $this;
    }
    $saved_path = $this->_get_savedpath();
    $saved_name = $this->_get_savedname($filename);
    $saved_full = $saved_path . $saved_name;
    if( $this->file_create($this->_configs['webroot'] . $this->_configs['saved_path'] . $saved_path)
        and $this->file_move($file['tmp_name'], $this->_configs['webroot'] . $this->_configs['saved_path'] . $saved_full)
      ) {
      $this->_results['original'] = $file['name'];
      $this->_results['name']     = $saved_name;
      $this->_results['url']      = $saved_full;
      $this->_results['size']     = $file['size'];
      $this->_results['type']     = '.' . $this->_file_ext($filename);
      $this->_results['state']    = 'SUCCESS';
    }
    else {
      $this->_results['state'] = 'have not enough folder permission';
    }
    return $this;
  }
  public function needStyle() {
    $settings = variable_get('ueditor_image_settings', NULL);
    $settings = $settings ? unserialize($settings) : NULL;
    $this->settings = $settings;
    return $settings && isset($settings['styleNeed']);
  }
  public function getStyleImageUrl($url) {
    $style = $this->getWatermarkStyle();
    $styleUrl = 'styles/'.$style['name'].'/'.$url;
    $source_uri = 'public://ueditor/'.$url;
    $target_uri = 'public://ueditor/'.$styleUrl;
    image_style_create_derivative($style, $source_uri, $target_uri);
    return $styleUrl;
  }
  public function getWatermarkStyle() {
    $cfg = ueditor_image_get_user_config();
    if (!empty($cfg)) {
      $u_cfg = $cfg;
      if ($u_cfg['watermarkNeed']) {
        // 0 is not choose
        if ($u_cfg['watermarkType'] == 1) {
          // system
          $style = image_style_load($u_cfg['watermarkSystem']);
          if ($style) {
            return $style;
          }
        }
        else if ($u_cfg['watermarkType'] == 2) {
          // custom
          $style = image_style_load($this->settings['watermarkCustomStyleBase']);
          if ($style) {
            $x_data = array(0 => 'left', 1 => 'center', 2 => 'right');
            $y_data = array(0 => 'top', 1 => 'center', 2 => 'bottom-5');
            $x_align_data = array(0 => 'left', 1 => 'center', 2 => 'right');
            $y_align_data = array(0 => 'top', 1 => 'center', 2 => 'bottom');
            $x = $x_data[$u_cfg['watermarkCutomX']];
            $x_align = $x_align_data[$u_cfg['watermarkCutomX']];
            $y_align = $y_align_data[$u_cfg['watermarkCutomY']];
            $y = $y_data[$u_cfg['watermarkCutomY']];
            $text = empty($u_cfg['watermarkCutomText']) ? $this->settings['watermarkDefaultText'] : $u_cfg['watermarkCutomText'];
            foreach ($style['effects'] as $key => $val) {
              if ($val['name'] == 'image_effects_text') {
                $style['effects'][$key]['data']['xpos'] = $x;
                $style['effects'][$key]['data']['ypos'] = $y;
                $style['effects'][$key]['data']['halign'] = $x_align;
                $style['effects'][$key]['data']['valign'] = $y_align;
                $style['effects'][$key]['data']['text'] = $text;
                break;
              }
            }
            return $style;
          }
        }
      }
    }
    return image_style_load($this->settings['styleDefault']);
  }
  public function getResult() {
    if ($this->_configs['file_type'] == 'image' && $this->needStyle()) {
      $this->_results['url'] = $this->getStyleImageUrl($this->_results['url']);
    }
    return $this->_results;
  }
  private function base64File() {
    if(empty($_POST[ $this->_configs['file_handle'] ])) {
      $this->_results['state'] = 'unvalidate form data';
      return ;
    }
    $b64 = $_POST[ $this->_configs['file_handle'] ];
    $img = base64_decode($b64);
    if(substr($img,0,8) == chr(137).'PNG'.chr(13).chr(10).chr(26).chr(10))
      $ext = '.png';
    else
      $ext = '.jpg';
    $size = strlen($img);
    $saved_path = $this->_get_savedpath();
    $saved_name = $this->_get_savedname('0'.$ext);
    $saved_full = $saved_path . $saved_name;
    if( !$this->_is_allowed_size($size) ) {
      $this->_results['state'] = 'filesize out of range';
      return ;
    }
    if( $this->file_create($this->_configs['webroot'] . $this->_configs['saved_path'] . $saved_full, $img) ) {
      $this->_results['original'] = '';
      $this->_results['name']     = $saved_name;
      $this->_results['url']      = $saved_full;
      $this->_results['size']     = $size;
      $this->_results['type']     = $ext;
      $this->_results['state']    = 'SUCCESS';
    }
    else {
      $this->_results['state'] = 'have not enough folder permission';
    }
  }
  
  private function _is_legal($file=array()) {
    foreach($this->_keyarr as $key) {
      if(!isset($file[$key])) {
        return FALSE;
      }
    }
    return TRUE;
  }
  private function _is_image($filename) {
    return in_array($this->_file_ext($filename), $this->_images);
  }
  private function _get_savedpath() {
    return date('Ymd/');
  }
  private function _get_savedname($filename) {
    if( isset($this->_configs['saved_name']) )
      return $this->_configs['saved_name'] . '.' . $this->_file_ext($filename);
    else
      return  time() . '_' . $this->_random(6) . '.' . $this->_file_ext($filename);
  }
  private function _is_allowed_exts($filename) {
    return in_array($this->_file_ext($filename), $this->_configs['allowed_exts']);
  }
  private function _is_allowed_size($size) {
    return ((int)$size <= $this->_configs['allowed_size']);
  }
  private function _file_ext($filename) {
    $arr = explode('.', $filename);
    return strtolower(end($arr));
  }
  private function _random($length=6, $chars='0123456789') {
    $hash = '';
    $max  = strlen($chars) - 1;
    for($i = 0; $i < $length; $i++) {
      $hash .= $chars[mt_rand(0, $max)];
    }
    return $hash;
  }
  private function file_move($file_name, $target_name='') {
    if (function_exists("move_uploaded_file")) {
      if (move_uploaded_file($file_name, $target_name)) {
        return true;
      }
      elseif (copy($file_name, $target_name)) {
        return true;
      }
    }
    elseif (copy($file_name, $target_name)) {
      return true;
    }
    return false;
  }
  private function file_create($folder, $data=' ') {
    $folder = strtr($folder,'\\','/');
    if(substr($folder,-1)=='/') {
      $path = rtrim($folder,'/');
      if(!is_dir($path)) return mkdir($path,0777,true);
      return true;
    }
    else {
      $path = dirname($folder);
      !is_dir($path) and mkdir($path,0777,true);
      return file_put_contents($folder, $data);
    }
  }
}
