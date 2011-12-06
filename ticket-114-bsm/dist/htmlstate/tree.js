function toggleFolder(id, imageNode) {
  var folder = document.getElementById(id);
  var l = imageNode.src.length;
  if (imageNode.src.substring(l-23,l)=="icon_folder_closed.png" || imageNode.src.substring(l-20,l)=="icon_folder_open.png") {
    imageNode = imageNode.previousSibling;
    l = imageNode.src.length;
  }
  if (folder == null) {
  } 
  else if (folder.style.display == "block") {
    if (imageNode != null) {
      imageNode.nextSibling.src = "icon_folder_closed.png";
      if (imageNode.src.substring(l-14,l) == "icon_mnode.png") {
        imageNode.src = "icon_pnode.png";
      }
      else if (imageNode.src.substring(l-18,l) == "icon_mlastnode.png") {
        imageNode.src = "icon_plastnode.png";
      }
    }
    folder.style.display = "none";
  }
  else {
    if (imageNode != null) {
      imageNode.nextSibling.src = "icon_folder_open.png";
      if (imageNode.src.substring(l-14,l) == "icon_pnode.png") {
        imageNode.src = "icon_mnode.png";
      }
      else if (imageNode.src.substring(l-18,l) == "icon_plastnode.png") {
        imageNode.src = "icon_mlastnode.png";
      }
    }
    folder.style.display = "block";
  }
}

function openAll() {
  for (e=1; ; e++) {
    var folder = document.getElementById("section-"+e);
    if (folder == null) {
      return ;
    }
    var ocNode = folder.previousSibling.previousSibling.lastChild.previousSibling.previousSibling.previousSibling;
    var mpNode = ocNode.previousSibling;
    var ocl = ocNode.src.length;
    var mpl = mpNode.src.length;
    folder.style.display = "block";
    if (mpNode.src.substring(mpl-14,mpl) == "icon_pnode.png") {
      mpNode.src = "icon_mnode.png";
    }
    else if (mpNode.src.substring(mpl-18,mpl) == "icon_plastnode.png") {
      mpNode.src = "icon_mlastnode.png";
    }

    if (ocNode.src.substring(ocl-22,ocl) == "icon_folder_closed.png") {
      ocNode.src = "icon_folder_open.png";
    }
  }
}

function closeAll() {
  for (e=1; ; e++) {
    var folder = document.getElementById("section-"+e);
    if (folder == null) {
      return ;
    }
    var ocNode = folder.previousSibling.previousSibling.lastChild.previousSibling.previousSibling.previousSibling;
    var mpNode = ocNode.previousSibling;
    var ocl = ocNode.src.length;
    var mpl = mpNode.src.length;
    folder.style.display = "none";
    if (mpNode.src.substring(mpl-14,mpl) == "icon_mnode.png") {
      mpNode.src = "icon_pnode.png";
    }
    else if (mpNode.src.substring(mpl-18,mpl) == "icon_mlastnode.png") {
      mpNode.src = "icon_plastnode.png";
    }
    if (ocNode.src.substring(ocl-20,ocl) == "icon_folder_open.png") {
      ocNode.src = "icon_folder_closed.png";
    }
  }
}
