[@bs.val] external hot: bool = "module.hot";

[@bs.val] external accept: unit => unit = "module.hot.accept";
[@bs.val] external import: string => Js.Promise.t('a) = "import";

let dictSet = (dict, key, value) => {
  Js.Dict.set(dict, key, value);
  dict;
};

let log = (something, ~label="", ()) => {
  Js.log2(label, something);
  something;
};

let compareDates = (a, b) => {
  let aTime = Js.Date.getTime(a);
  let bTime = Js.Date.getTime(b);

  if (aTime > bTime) {
    1;
  } else if (aTime < bTime) {
    (-1);
  } else {
    0;
  };
};

let nanoIdAlphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
[@bs.module]
external generateNanoId: (string, int) => string = "nanoid/generate";
let generateId = () => generateNanoId(nanoIdAlphabet, 10);

let find = (xs: list('a), predicate: 'a => bool): option('a) =>
  Belt.List.keep(xs, predicate)->Belt.List.head;

let textToHtml = text =>
  Js.String.replaceByRe([%re "/(?:\\r\\n|\\r|\\n)/g"], "<br/>", text);

let htmlToText: string => string = [%bs.raw
  {|
// https://github.com/eldios/htmlToText/blob/master/jsHtmlToText.js
function htmlToText(html) {
  return html
    // Remove line breaks
    .replace(/(?:\n|\r\n|\r)/ig,"")
    // Turn <br>'s into single line breaks.
    .replace(/<\s*br[^>]*>/ig,"\n")
    // Turn </li>'s into line breaks.
     .replace(/<\s*\/li[^>]*>/ig,"\n")
    // Turn <p>'s into double line breaks.
     .replace(/<\s*p[^>]*>/ig,"\n\n")
    // Remove content in script tags.
     .replace(/<\s*script[^>]*>[\s\S]*?<\/script>/mig,"")
    // Remove content in style tags.
     .replace(/<\s*style[^>]*>[\s\S]*?<\/style>/mig,"")
    // Remove content in comments.
     .replace(/<!--.*?-->/mig,"")
     // Format anchor tags properly.
     // e.g.
     // input - <a class='ahref' href='http://pinetechlabs.com/' title='asdfqwer\"><b>asdf</b></a>
     // output - asdf (http://pinetechlabs.com/)
     .replace(/<\s*a[^>]*href=['"](.*?)['"][^>]*>([\s\S]*?)<\/\s*a\s*>/ig, "$2 ($1)")
    // Remove all remaining tags.
     .replace(/(<([^>]+)>)/ig,"")
    // Make sure there are never more than two
    // consecutive linebreaks.
     .replace(/\n{2,}/g,"\n\n")
    // Remove tabs.
     .replace(/\t/g,"")
    // Remove newlines at the beginning of the text.
     .replace(/^\n+/m,"")
    // Replace multiple spaces with a single space.
    .replace(/ {2,}/g," ");
}
|}
];

[@bs.module "classnames"] [@bs.splice]
external classnames: array(string) => string = "default";

module DayJs = {
  type t;
  [@bs.module "dayjs"] external make: Js.Date.t => t = "default";
  [@bs.send] external format: (t, string) => string = "format";
};

let formatDate = (date, format) => date->DayJs.make->DayJs.format(format);