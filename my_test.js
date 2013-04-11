/* 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

//(function($) {
//  // look through all the spp entry boxes
//  Drupal.behaviors.bbl = {
//    attach: function(context, settings) {
//      $(context).find("#player-spp-1").append("fred");  
//    }
//  }
//
//})(jQuery);


(function($) {
  // THIS WORKS!! 
  // first task for this code is input validation as only numerical values can 
  // be entered
  // Second task is to total the spp for each player
  Drupal.behaviors.bbl = {
    attach: function(context, settings) {
      $(context).find("form :text").filter('input[name^="players"]').change( function () {
        var fred = $(this).val();
        if ( parseInt(fred, 10) == 'NaN') {
          $(this).val('0');
        } else {
          $(this).val( parseInt(fred, 10) );
        }
        
        // retrieve player id from clicked text element
        var patt = new RegExp("[0-9]+");
        var fieldname = $(this).attr('name');
        var playerid = fieldname.match(patt);
        
        // find the name for this player line in the form 'player[1]'
        var pid_filter = 'input[name^="players[' + playerid + ']"]';
        var tot_field = "#player-spp-" + playerid;
        var total = new Number(0);
        
        // Loop along row summing the players total spp (tpid is a hidden field)
        $( pid_filter ).not('[name$="[tpid]"]').each(function(  ) {
          var spp = $(this).attr('spp_value');
          var award = parseInt($(this).val(), 10);
          total += spp * award;
        });
        
        $( tot_field ).text( total ); 
      });
    }
  }

})(jQuery);