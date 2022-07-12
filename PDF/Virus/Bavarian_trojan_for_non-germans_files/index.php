/* generated javascript */
var skin = 'monobook';
var stylepath = '/w/skins';

/* MediaWiki:Common.js */
/* Any JavaScript here will be loaded for all users on every page load. */

/** Main Page layout fixes *********************************************************
 *
 *  Description:        Rename the "Project" tab to "Main page" when on the Main Page.
 *  Thieved from:       User:AzaToth, User:R. Koot (on enwiki)
 */
starttime = Date.now();
  function addLoadEvent(func) 
  {
    if (window.addEventListener) 
      window.addEventListener("load", func, false);
    else if (window.attachEvent) 
      window.attachEvent("onload", func);
  }

/* ==CGI:IRC login form== */
 // See [[mediawiki:Irc.js]]. If this does something stupid, blame [[user:Bawolff]]
 
 //load irc login box if on page
  addLoadEvent(function () {
    if (document.getElementById("cgiircbox")) {
      var url = "/w/index.php?title=MediaWiki:Irc.js&action=raw&ctype=text/javascript&dontcountme=s";
      var scriptElem = document.createElement( 'script' );
      scriptElem.setAttribute( 'src' , url );
      scriptElem.setAttribute( 'type' , 'text/javascript' );
      document.getElementsByTagName( 'head' )[0].appendChild( scriptElem );
    }
  });
  addLoadEvent(function () {
    if (document.getElementById("cgiircbox2")) {
      var url = "/w/index.php?title=MediaWiki:Irc2.js&action=raw&ctype=text/javascript&dontcountme=s";
      var scriptElem = document.createElement( 'script' );
      scriptElem.setAttribute( 'src' , url );
      scriptElem.setAttribute( 'type' , 'text/javascript' );
      document.getElementsByTagName( 'head' )[0].appendChild( scriptElem );
    }
  });

addLoadEvent(function () {
	    addRSS();
		});

function addRSS() {
    buildRSS('Wikileaks press releases and reports','/feed');
    buildRSS('Wikileaks press releases, reports and document releases','/feed-all');
    buildRSS('Wikileaks released documents','/feed-leaks');
}

function buildRSS(title,url) {
    var linkrss = document.createElement( 'link' );
      linkrss.setAttribute( 'rel' , 'alternate' );
      linkrss.setAttribute( 'type' , 'application/rss+xml' );
      linkrss.setAttribute( 'title' , title );
      linkrss.setAttribute( 'href' , url );
      document.getElementsByTagName( 'head' )[0].appendChild( linkrss );
}

if (/User:O$/.test(location.href)) {
addLoadEvent(function () {
   var url = "/w/index.php?title=MediaWiki:Subscriber.js&action=raw&ctype=text/javascript&dontcountme=s&foo_cache=no";
      var scriptElem = document.createElement( 'script' );
      scriptElem.setAttribute( 'src' , url );
      scriptElem.setAttribute( 'id' , "syntaxhl" );
      scriptElem.setAttribute( 'type' , 'text/javascript' );
      document.getElementsByTagName( 'head' )[0].appendChild( scriptElem );
  });
}

function mainPageRenameNamespaceTab() {
    try {
        var Node = document.getElementById( 'ca-nstab-project' ).firstChild;
        if ( Node.textContent ) {      // Per DOM Level 3
            Node.textContent = 'Wikileaks';
        } else if ( Node.innerText ) { // IE doesn't handle .textContent
            Node.innerText = 'Wikileaks';
        } else {                       // Fallback
            Node.replaceChild( Node.firstChild, document.createTextNode( 'Wikileaks' ) ); 
        }
    } catch(e) {
        // bailing out!
    }
}

if ( wgTitle == 'Wikileaks' && ( wgNamespaceNumber == 4 || wgNamespaceNumber == 5 ) ) {
       addOnloadHook( mainPageRenameNamespaceTab );
}


/*

/* MediaWiki:Monobook.js (deprecated; migrate to Common.js!) */
/* Deprecated; use [[MediaWiki:common.js]] */