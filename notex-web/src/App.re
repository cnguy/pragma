open Belt;

[@bs.module] external styles: Js.Dict.t(string) = "./index.scss";
let style = name => Js.Dict.get(styles, name)->Option.getExn;

open Data;

type noteCount = int;
type notebookWithCount = (notebook, noteCount);

type state = {
  notebooks: option(list(notebookWithCount)),
  selectedNotebook: option(int),
  notes: option(list(note)),
  selectedNote: option(int),
  contentBlocks: option(list(contentBlock)),
};

type action =
  | Load(state)
  | SelectNote(int)
  | LoadNote(int, list(contentBlock))
  | SelectNotebook(int)
  | LoadNotebook(int, list(note), option(int), option(list(contentBlock)))
  | UpdateNoteText(contentBlock, string);

let sortDesc = (notes: list(note)) =>
  Belt.List.sort(notes, (a, b) =>
    Utils.compareDates(a.updatedAt, b.updatedAt) * (-1)
  );

let getNotes = notebookId =>
  Db.getNotes(notebookId)
  ->Future.map(notes => {
      let selectedNoteId = List.head(notes)->Option.map(note => note.id);
      (notes, selectedNoteId);
    })
  ->Future.flatMap(((notes, selectedNoteId)) =>
      switch (selectedNoteId) {
      | None => Future.value((notes, selectedNoteId, None))
      | Some(noteId) =>
        Db.getContentBlocks(noteId)
        ->Future.map(contentBlocks =>
            (notes, selectedNoteId, Some(contentBlocks))
          )
      }
    );

module MainUI = {
  let renderNotebooks =
      (
        notebooks: list(notebookWithCount),
        selectedNotebook: option(int),
        send,
      ) => {
    let listItems =
      List.map(notebooks, ((notebook, noteCount)) =>
        (
          {
            id: notebook.id |> string_of_int,
            title: notebook.name,
            count: Some(noteCount),
            model: notebook,
          }:
            ListView.listItem(notebook)
        )
      );

    <ListView
      items=listItems
      selectedId={Option.map(selectedNotebook, string_of_int)}
      onItemSelected={item => send(SelectNotebook(item.model.id))}
    />;
  };

  let renderNotes =
      (notes: option(list(note)), selectedNote: option(int), send) => {
    let listItems =
      switch (notes) {
      | None => []
      | Some(notes) =>
        List.map(notes, note =>
          (
            {
              id: note.id |> string_of_int,
              title: note.title,
              count: None,
              model: note,
            }:
              ListView.listItem(note)
          )
        )
      };

    let formatDate = date => DateFns.format("D MMMM YYYY", date);
    let renderNoteListItemContent = (item: ListView.listItem(note)) =>
      <p>
        {ReasonReact.string(item.model.title)}
        <br />
        <small>
          {ReasonReact.string(item.model.updatedAt |> formatDate)}
        </small>
      </p>;

    <ListView
      minWidth="250px"
      items=listItems
      selectedId={Option.map(selectedNote, string_of_int)}
      onItemSelected={item => send(SelectNote(item.model.id))}
      renderItemContent=renderNoteListItemContent
    />;
  };

  let component = ReasonReact.statelessComponent("MainUI");
  let make =
      (
        ~notebooks,
        ~selectedNotebook,
        ~notes,
        ~selectedNote,
        ~contentBlocks,
        ~editingNote,
        ~send,
        _children,
      ) => {
    ...component,
    render: _self =>
      <main className={style("main")}>
        <div className={style("columns")}>
          {renderNotebooks(notebooks, selectedNotebook, send)}
          {renderNotes(notes, selectedNote, send)}
          <NoteEditor
            note=editingNote
            contentBlocks
            onChange={
              (contentBlock, value) =>
                send(UpdateNoteText(contentBlock, value))
            }
          />
        </div>
      </main>,
  };
};

let reducer = (action: action, state: state) =>
  switch (action) {
  | Load(state) => ReasonReact.Update(state)
  | SelectNotebook(notebookId) =>
    ReasonReact.SideEffects(
      (
        self =>
          getNotes(notebookId)
          ->Future.get(((notes, selectedNoteId, contentBlocks)) =>
              self.send(
                LoadNotebook(
                  notebookId,
                  notes,
                  selectedNoteId,
                  contentBlocks,
                ),
              )
            )
      ),
    )

  | LoadNotebook(notebookId, notes, selectedNoteId, contentBlocks) =>
    {
      ...state,
      selectedNotebook: Some(notebookId),
      notes: Some(notes),
      selectedNote: selectedNoteId,
      contentBlocks,
    }
    ->ReasonReact.Update

  | SelectNote(noteId) =>
    ReasonReact.SideEffects(
      (
        self =>
          Db.getContentBlocks(noteId)
          ->Future.get(contentBlocks =>
              self.send(LoadNote(noteId, contentBlocks))
            )
      ),
    )

  | LoadNote(noteId, contentBlocks) =>
    {
      ...state,
      selectedNote: Some(noteId),
      contentBlocks: Some(contentBlocks),
    }
    ->ReasonReact.Update

  | UpdateNoteText(contentBlock, text) =>
    let updatedContentBlock =
      switch (contentBlock.content) {
      | TextContent(_) => {...contentBlock, content: TextContent(text)}
      | _ => Js.Exn.raiseError("TODO")
      };

    ReasonReact.SideEffects(
      (_self => Db.updateContentBlock(updatedContentBlock, ()) |> ignore),
    );
  };

let component = ReasonReact.reducerComponent("App");
let make = _children => {
  ...component,
  initialState: () => {
    notebooks: None,
    selectedNotebook: None,
    notes: None,
    selectedNote: None,
    contentBlocks: None,
  },
  reducer,
  didMount: self => {
    let fetchData = () => {
      let appState = AppState.get();

      Db.getNotebooks()
      ->Future.flatMap(notebooks => {
          let selectedNotebookId =
            switch (appState.selectedNotebookId) {
            | Some(id) => Some(id)
            | None =>
              List.head(notebooks)
              ->Belt.Option.map(((notebook, _count)) => notebook.id)
            };

          switch (selectedNotebookId) {
          | None => Future.value((notebooks, None, None))
          | Some(notebookId) =>
            Db.getNotes(notebookId)
            ->Future.map(notes =>
                (notebooks, Some(notebookId), Some(notes))
              )
          };
        })
      ->Future.get(((notebooks, selectedNotebook, notes)) => {
          let selectedNoteId =
            switch (appState.selectedNoteId) {
            | Some(id) => Some(id)
            | None =>
              Belt.Option.flatMap(notes, notes =>
                List.head(notes)->Option.map(note => note.id)
              )
            };

          let contentBlocksFuture =
            switch (selectedNoteId) {
            | None => Future.value([])
            | Some(noteId) => Db.getContentBlocks(noteId)
            };

          contentBlocksFuture
          ->Future.get(contentBlocks =>
              self.send(
                Load({
                  notebooks: Some(notebooks),
                  notes,
                  selectedNotebook,
                  selectedNote: selectedNoteId,
                  contentBlocks: Some(contentBlocks),
                }),
              )
            );
        });
    };

    fetchData();
    Db.subscribe(fetchData);
  },
  didUpdate: ({oldSelf: _oldSelf, newSelf}) =>
    AppState.setSelected(
      newSelf.state.selectedNotebook,
      newSelf.state.selectedNote,
    ),
  render: self => {
    let editingNote =
      self.state.selectedNote
      ->Option.flatMap(selectedNote =>
          switch (self.state.notes) {
          | None => None
          | Some(notes) =>
            Belt.List.keep(notes, n => n.id == selectedNote)->List.head
          }
        );

    switch (self.state.notebooks) {
    | None => <div> {ReasonReact.string("Loading...")} </div>
    | Some(_notebooks) =>
      <MainUI
        notebooks=self.state.notebooks->Option.getExn
        selectedNotebook={self.state.selectedNotebook}
        notes={self.state.notes}
        selectedNote={self.state.selectedNote}
        contentBlocks={self.state.contentBlocks}
        editingNote
        send={self.send}
      />
    };
  },
};
