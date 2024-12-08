// Small utility JS module to allow single line comments in JSON
export const parse = text => JSON.parse(removeComment(text));

export const removeComment = text =>
  text.replaceAll(
    /("(?:\\"|\\.|[^"])*"|[^\/])|\/\/.*|(\/)/g,
    "$1$2"
  );
