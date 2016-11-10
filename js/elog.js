// Reclaim $ alias for jquery
(function ($) {
    var urlVars = getUrlVars();
    Drupal.behaviors.elog = {
        attach: function (context, settings) {
            // Advanced filters form elements   
            if (jQuery().select2) {
                $(".select2").select2({width: '200px'});
                $(".select2-m").select2({width: '400px'});
                $(".select2-l").select2({width: '600px'});
                $(".select2-xl").select2({width: '800px'});
            }else{
                console.log('select2 plugin not loaded!');
            }
            if (jQuery().datetimepicker) {
              $(".datetimepicker").datetimepicker({
                'format': 'Y-m-d H:i'
              });
            }else{
              console.log('datetimepicker plugin not loaded!');
            }
            
            // Link the exclude_tags_select and exclude_autologs_checkbox
            // so they stay in sync.
            // First code that updates the checkbox when the select is changed.
            $('#exclude_tags_select').on('change', function(){
               var isAutolog = false;
               $('#exclude_tags_select option:selected').each(function(){
                   if ($(this).text() == 'Autolog'){
                       isAutolog = true;
                   }
                });
                $('#exclude_autologs_checkbox').prop('checked', isAutolog);
            });
            // And then code to update the select when the checkbox changes.
            $('#exclude_autologs_checkbox').on('change', function(){
               var options = [];
               $('#exclude_tags_select option').each(function(){
                   if ($(this).text() == 'Autolog'){
                       if ($('#exclude_autologs_checkbox').prop('checked')){
                           options.push($(this).val());
                       }
                   }else if ($(this).prop('selected')){
                       options.push($(this).val());
                   }
                });
              $('#exclude_tags_select').select2('val',options);
            });

            
            // Below is the sidebar datepicker   
            $('#elog-calendar').css('color', 'green');
            $('#elog-calendar').datepicker({
                inline: true,
                dateFormat: "yy-mm-dd", // is really YYYY-MM-DD!!!
                onSelect: function (selectedDate, $inst) {
                    //console.log('selected date is ' + selectedDate);
                    //Put dates into inputs and click apply.
                    $('input[name="start_date[date]"]').val('');
                    $('input[name="start_date[time]"]').val('');
                    $('input[name="end_date[date]"]').val(selectedDate);
                    $('input[name="end_date[time]"]').val('23:59');
                   
                    $('input[name="start_date"]').val('');
                   
                    var endDate = new Date(selectedDate);
                    // Note the difference:
                    // getUTCMonth() returns 0-11
                    // getUTCDate()  returns 1-31
                    var endDateString = 
                            endDate.getUTCFullYear() +"-"+
                            ("0" + (endDate.getUTCMonth()+1)).slice(-2) +"-"+
                            ("0" + (endDate.getUTCDate())).slice(-2) + 
                            " 23:59";

                    $('input[name="end_date"]').val(endDateString);
                    
                    if ($('#elog-form-advanced-filters')){
                      $('#elog-form-advanced-filters').submit();
                    }else if ( $('#elog-daterange-form')){
                      $('#elog-daterange-form').submit();
                    }else{
                      alert('The Date Picker block requires the Date Range or Filter Form blocks!');
                    }
                    //$('#elog-form-advanced-filters input[type="submit"]').click();
                    //$('#elog-daterange-form input[type="submit"]').click();
                    // console.log('bye');
                }
            });
            // The %5B,%5D is the urlencoded [] around the url var.    
            if (urlVars['end_date']) {
                var d = decodeURIComponent(urlVars['end_date']);
                //console.log('d is'+d);
                $('#elog-calendar').datepicker('setDate', d);
            }
        }
    };

})(jQuery);


/**
 * Javascript can't seem to handle yyyy-mm-dd in Date constructor string
 */
function makeDPDate(str) {
//  var YYYY = str.substring(0, 4);
//  var MM = str.substring(5, 7);
//  var DD = str.substring(8);
//  var myDate = new Date(parseInt(YYYY, 10), parseInt(MM, 10) - 1, parseInt(DD, 10)); 
//  //alert(YYYY+' '+MM+' '+DD);
//  return myDate; // should be november 27th 2010
}


// Read a page's GET URL variables and return them as an associative array.
function getUrlVars()
{
    var vars = [], hash;
    var hashes = window.location.href.slice(window.location.href.indexOf('?') + 1).split('&');
    for (var i = 0; i < hashes.length; i++)
    {
        hash = hashes[i].split('=');
        vars.push(hash[0]);
        vars[hash[0]] = hash[1];
    }
    return vars;
}



/**
 * Auto-Refresh and related timer stuff in next section 
 */


/**
 * Initialize autorefresh
 */
jQuery(document).ready(function () {
    jQuery("#doLogbookRefresh").on('click', updateAutoRefreshState);
    //Initialize with first setAutoRefreshState invocation
    initializeDoLogbookRefresh(getAutoRefreshStateCookie());
    setAutoRefreshState(getAutoRefreshStateCookie());
    setLogRefresh(refreshMaster);
});


/**
 * Issues ajax load request for the content div, but only if
 * the current URL is "safe", meaning it doesn't contain
 * any query parameters and is of a recognizable pattern able
 * to be refreshed.
 */
function doLogbookRefresh() {
    var regex = /^(([^:/?#]+):)?(\/\/([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?/
    var urlParts = window.location.href.match(regex);
    var urlPath = urlParts[5];
    var urlQuery = urlParts[6];
    var refreshOK = urlPath.match(/^(\/((book|tag|entries)(\/([\w]+))?)?)$/)
    if (refreshOK) {
        document.location.reload(true);
    }
//  }
}


var refreshMaster = 15;          // Minutes between AutoRefresh
var refreshInterval;             // Remaining Minutes to wait until AutoRefresh
var secondsToWait;              // Seconds remaining once countdown begins
var qf_timer;                   // Javascript TimeOut object used to countdown refresh
var refreshIntervalDivOrig;     //stores state of refreshIntervalDiv priot to user disablement.



/**
 * Set the time until next autorefresh
 */
function setLogRefresh(minutesToWait) {
    if (minutesToWait >= 1) {
        refreshInterval = minutesToWait;
        secondsToWait = minutesToWait * 60;
        jQuery('#refreshMinutesSpan').html(secondsToWait / 60);
        startClock();
    }
}


function startClock() {
    secondsToWait = secondsToWait - 1;
    // Update an indicator once/minute 
    if (secondsToWait % 60 == 0 && refreshInterval >= 1) {
        jQuery('#refreshMinutesSpan').html(secondsToWait / 60);
    }

    //refreshInterval of 0 means user has disabled autorefresh
    if (secondsToWait == 0 && jQuery("#doLogbookRefresh").prop('checked') == 1) {
        doLogbookRefresh();
        setLogRefresh(refreshMaster)
    } else {
        qf_timer = setTimeout("startClock()", 1000);
    }
}

// Respond to a checkbox click to change refresh state
function updateAutoRefreshState() {
    if (jQuery(this).prop('checked')) {
        setAutoRefreshState(1);
    } else {
        setAutoRefreshState(0);
    }
}


function getAutoRefreshStateCookie() {
    var ret = jQuery.cookie("logbooksRefresh");
    return ret;
}


function initializeDoLogbookRefresh(stateOption) {
    if (stateOption == 1) {
        jQuery("#doLogbookRefresh").prop('checked', 1);
    } else {
        jQuery("#doLogbookRefresh").prop('checked', 0);
    }
}

function setAutoRefreshStateCookie(stateOption) {
    //console.log('Set state to '+stateOption);
    jQuery.cookie("logbooksRefresh", stateOption, {
        expires: 10, //expires in 10 days
        path: '/',
        secure: false
    });

}

/**
 * Enables or Disables autorefresh at user's request.
 */
function setAutoRefreshState(stateOption) {
    if (stateOption == 1) {
        jQuery("#refreshStatusDivDisabled").hide();
        jQuery("#refreshStatusDivEnabled").show();
        setLogRefresh(refreshMaster);
    } else {
        jQuery("#refreshStatusDivEnabled").hide();
        jQuery("#refreshStatusDivDisabled").show();
        refreshInterval = 0;
        clearTimeout(qf_timer);
    }
    setAutoRefreshStateCookie(stateOption);
}






