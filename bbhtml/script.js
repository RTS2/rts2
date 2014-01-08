// script window for Schedule UI
$(function() {
	$("#script-form").dialog({
		autoOpen: false,
		modal: true,
		width: 300,
		height: 400,
		buttons: {
			'Save': function() {
				$(this).dialog('close');
			},
			'Cancel': function() {
				$(this).dialog('close');
			}
		}
	});

	$("#script_add_add").click(function() {
		alert('Trying to add. Exposure ' + $('#script.add.exposure').value);
	})
});
