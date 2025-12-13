window.onload = () => {
	let queryParameters = new URL(document.location.toString()).searchParams;

	const showId = queryParameters.has("ok") ? "imageDiv" : "formDiv";
	const element = document.getElementById(showId);
	element.hidden = false;

	onNodeTypeChange();
}

function onNodeTypeChange() {
	let nodeType = document.querySelector('input[name="nodeType"]:checked').value;

	const fieldSet = document.getElementById("gatewayFieldset");
	if(nodeType === "gateway") {
		fieldSet.style.display = "flex";
		fieldSet.hidden = false;
	} else {
		fieldSet.style.display = "none";
		fieldSet.hidden = true;
	}
}
