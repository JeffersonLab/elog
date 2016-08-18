
(function ($) {
  
  Drupal.behaviors.elog_widgets = {
    attach: function (context, settings) {
      var config = settings.elog_taxonomy_select2;
      if (typeof config != 'undefined' && typeof config.elements != 'undefined') {
        for (var el in config.elements) {
          var e = $('#' + config.elements[el].id, context);
          e.select2({
            width: '350px'
          });
        }
      }
    }
  };

})(jQuery);
