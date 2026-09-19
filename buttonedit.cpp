#include "buttonedit.h"
#include <functional>
#include <string>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QInputMethod>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QValidator>
#include <QVBoxLayout>
#include "inlinepopup.h"
#include "qutil.h"

ButtonEditable::ButtonEditable(
    const std::string &initial_button_value,
    QWidget *parent,
    const std::string &dialog_title,
    const std::string &dialog_prompt,
    get_str_t get_edit_value,
    QValidator *validator,
    parser_t parse) :
      QPushButton{QString::fromStdString(initial_button_value), parent},
      initial_button_value_{initial_button_value},
      dialog_title_{dialog_title},
      dialog_prompt_{dialog_prompt},
      validator_{validator},
      get_edit_value_{get_edit_value},
      parse_{parse} {
  if (validator_) {
    validator_->setParent(this);
  }
  connect(this, &QPushButton::clicked, this, &ButtonEditable::Edit);
}

void ButtonEditable::Edit() {
  InlinePopup popup(window());
  QWidget *panel = popup.ContentPanel();

  auto layout = new QVBoxLayout(panel);
  auto prompt = new QLabel{QString::fromStdString(dialog_prompt_), panel};
  const auto s = QString::fromStdString(get_edit_value_());
  auto edit = new QLineEdit(s, panel);
  if (validator_) {
    edit->setValidator(validator_);
  }
  layout->addWidget(prompt);
  layout->addWidget(edit);

  auto error_label = new QLabel(panel);
  error_label->setStyleSheet("color: red;");
  layout->addWidget(error_label);

  QDialogButtonBox *buttons = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, panel);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &popup,
      &InlinePopup::Accept);
  connect(buttons, &QDialogButtonBox::rejected, &popup,
      &InlinePopup::Reject);
  edit->setFocus();
  QGuiApplication::inputMethod()->show();

  bool done = false;
  int exec_rc = -1;
  std::string parse_error{"dummy-non-empty"};
  qDebug() << std::format("Accepted={}", int(InlinePopup::Accepted));
  while ((!parse_error.empty()) 
      && ((exec_rc = popup.Exec()) == InlinePopup::Accepted)) {
    const auto qs = edit->text();
    const auto s = qs.toStdString();
    std::string text_to_set;
    parse_error = parse_(s, text_to_set);
    error_label->setText(QString::fromStdString(parse_error));
    if (parse_error.empty()) {
      setText(QString::fromStdString(text_to_set));
    }
  }
}
