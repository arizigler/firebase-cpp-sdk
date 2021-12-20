//  Copyright © 2021 Google. All rights reserved.

#include "gma/tools/ios/testapp/testapp/game_engine.h"

#include <cstddef>

#include "app/src/assert.h"

// AdMob app ID.
const char* kAdMobAppID = "ca-app-pub-3940256099942544~1458002511";

// AdMob ad unit IDs.
const char* kBannerAdUnit = "ca-app-pub-3940256099942544/2934735716";
const char* kInterstitialAdUnit = "ca-app-pub-3940256099942544/4411468910";

// A simple listener that logs changes to a BannerView.
class LoggingBannerViewListener : public firebase::gma::BannerView::Listener {
 public:
  LoggingBannerViewListener() {}
  void OnPresentationStateChanged(
      firebase::gma::BannerView* banner_view,
      firebase::gma::BannerView::PresentationState state) override {
    LogMessage("BannerView PresentationState has changed to %d.", state);
  }
  void OnBoundingBoxChanged(firebase::gma::BannerView* banner_view,
                            firebase::gma::BoundingBox box) override {
    LogMessage(
        "BannerView BoundingBox has changed to (x: %d, y: %d, width: %d, "
        "height %d)",
        box.x, box.y, box.width, box.height);
  }
};

// A simple listener that logs changes to an InterstitialAd.
class LoggingInterstitialAdListener
    : public firebase::gma::InterstitialAd::Listener {
 public:
  LoggingInterstitialAdListener() {}
  void OnPresentationStateChanged(
      firebase::gma::InterstitialAd* interstitial_ad,
      firebase::gma::InterstitialAd::PresentationState state) override {
    LogMessage("InterstitialAd PresentationState has changed to %d.", state);
  }
};

// The listeners for logging changes to the AdMob ad formats.
LoggingBannerViewListener banner_listener;
LoggingInterstitialAdListener interstitial_listener;

// GameEngine constructor.
GameEngine::GameEngine() {}

// Sets up GMA C++.
void GameEngine::Initialize(firebase::gma::AdParent ad_parent) {
  firebase::gma::Initialize(kAdMobAppID);
  parent_view_ = ad_parent;

  if (kTestBannerView) {
    // Create an ad size and initialize the BannerView.
    firebase::gma::AdSize bannerAdSize;
    bannerAdSize.width = 320;
    bannerAdSize.height = 50;
    banner_view_ = new firebase::gma::BannerView();
    banner_view_->Initialize(parent_view_, kBannerAdUnit, bannerAdSize);
    banner_view_listener_set_ = false;
  }
  if (kTestInterstitialAd) {
    // Initialize the InterstitialAd.
    interstitial_ad_ = new firebase::gma::InterstitialAd();
    interstitial_ad_->Initialize(parent_view_, kInterstitialAdUnit);
    interstitial_ad_listener_set_ = false;
  }
}

// Creates the GMA C++ ad request.
firebase::gma::AdRequest GameEngine::createRequest() {
  // Sample keywords to use in making the request.
  static const char* kKeywords[] = {"GMA", "C++", "Fun"};

  // Sample test device IDs to use in making the request.
  static const char* kTestDeviceIDs[] = {"2077ef9a63d2b398840261c8221a0c9b",
                                         "098fe087d987c9a878965454a65654d7"};

  // Sample birthday value to use in making the request.
  static const int kBirthdayDay = 10;
  static const int kBirthdayMonth = 11;
  static const int kBirthdayYear = 1976;

  firebase::gma::AdRequest request;
  request.gender = firebase::gma::kGenderUnknown;

  request.tagged_for_child_directed_treatment =
      firebase::gma::kChildDirectedTreatmentStateTagged;

  request.birthday_day = kBirthdayDay;
  request.birthday_month = kBirthdayMonth;
  request.birthday_year = kBirthdayYear;

  request.keyword_count = sizeof(kKeywords) / sizeof(kKeywords[0]);
  request.keywords = kKeywords;

  static const firebase::gma::KeyValuePair kRequestExtras[] = {
      {"the_name_of_an_extra", "the_value_for_that_extra"}};
  request.extras_count = sizeof(kRequestExtras) / sizeof(kRequestExtras[0]);
  request.extras = kRequestExtras;

  request.test_device_id_count =
      sizeof(kTestDeviceIDs) / sizeof(kTestDeviceIDs[0]);
  request.test_device_ids = kTestDeviceIDs;

  return request;
}

// Updates the game engine (game loop).
void GameEngine::onUpdate() {
  if (kTestBannerView) {
    // Set the banner view listener.
    if (banner_view_->InitializeLastResult().Status() ==
            firebase::kFutureStatusComplete &&
        banner_view_->InitializeLastResult().Error() ==
            firebase::gma::kAdErrorNone &&
        !banner_view_listener_set_) {
      banner_view_->SetListener(&banner_listener);
      banner_view_listener_set_ = true;
    }
  }

  if (kTestInterstitialAd) {
    // Set the interstitial ad listener.
    if (interstitial_ad_->InitializeLastResult().Status() ==
            firebase::kFutureStatusComplete &&
        interstitial_ad_->InitializeLastResult().Error() ==
            firebase::gma::kAdErrorNone &&
        !interstitial_ad_listener_set_) {
      interstitial_ad_->SetListener(&interstitial_listener);
      interstitial_ad_listener_set_ = true;
    }

    // Once the interstitial ad has been displayed to and dismissed by the user,
    // create a new interstitial ad.
    if (interstitial_ad_->ShowLastResult().Status() ==
            firebase::kFutureStatusComplete &&
        interstitial_ad_->ShowLastResult().Error() ==
            firebase::gma::kAdErrorNone &&
        interstitial_ad_->GetPresentationState() ==
            firebase::gma::InterstitialAd::kPresentationStateHidden) {
      delete interstitial_ad_;
      interstitial_ad_ = nullptr;
      interstitial_ad_ = new firebase::gma::InterstitialAd();
      interstitial_ad_->Initialize(parent_view_, kInterstitialAdUnit);
      interstitial_ad_listener_set_ = false;
    }
  }

  // Increment red if increasing, decrement otherwise.
  float diff = bg_intensity_increasing_ ? 0.0025f : -0.0025f;

  // Increment red up to 1.0, then back down to 0.0, repeat.
  bg_intensity_ += diff;
  if (bg_intensity_ >= 0.4f) {
    bg_intensity_increasing_ = false;
  } else if (bg_intensity_ <= 0.0f) {
    bg_intensity_increasing_ = true;
  }
}

// Handles user tapping on one of the kNumberOfButtons.
void GameEngine::onTap(float x, float y) {
  int button_number = -1;
  GLfloat viewport_x = 1 - (((width_ - x) * 2) / width_);
  GLfloat viewport_y = 1 - (((y)*2) / height_);

  for (int i = 0; i < kNumberOfButtons; i++) {
    if ((viewport_x >= vertices_[i * 8]) &&
        (viewport_x <= vertices_[i * 8 + 2]) &&
        (viewport_y <= vertices_[i * 8 + 1]) &&
        (viewport_y >= vertices_[i * 8 + 5])) {
      button_number = i;
      break;
    }
  }

  // The BannerView's bounding box.
  firebase::gma::BoundingBox box;

  switch (button_number) {
    case 0:
      if (kTestBannerView) {
        // Load the banner ad.
        if (banner_view_->InitializeLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            banner_view_->InitializeLastResult().Error() ==
                firebase::gma::kAdErrorNone) {
          banner_view_->LoadAd(createRequest());
        }
      }
      break;
    case 1:
      if (kTestBannerView) {
        // Show/Hide the BannerView.
        if (banner_view_->LoadAdLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            banner_view_->LoadAdLastResult().Error() ==
                firebase::gma::kAdErrorNone &&
            banner_view_->GetPresentationState() ==
                firebase::gma::BannerView::kPresentationStateHidden) {
          banner_view_->Show();
        } else if (banner_view_->LoadAdLastResult().Status() ==
                       firebase::kFutureStatusComplete &&
                   banner_view_->GetPresentationState() ==
                       firebase::gma::BannerView::
                           kPresentationStateVisibleWithAd) {
          banner_view_->Hide();
        }
      }
      break;
    case 2:
      if (kTestBannerView) {
        // Move the BannerView to a predefined position.
        if (banner_view_->LoadAdLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            banner_view_->LoadAdLastResult().Error() ==
                firebase::gma::kAdErrorNone) {
          banner_view_->MoveTo(firebase::gma::BannerView::kPositionBottom);
        }
      }
      break;
    case 3:
      if (kTestBannerView) {
        // Move the BannerView to a specific x and y coordinate.
        if (banner_view_->LoadAdLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            banner_view_->LoadAdLastResult().Error() ==
                firebase::gma::kAdErrorNone) {
          int x = 100;
          int y = 200;
          banner_view_->MoveTo(x, y);
        }
      }
      break;
    case 4:
      if (kTestInterstitialAd) {
        // Load the interstitial ad.
        if (interstitial_ad_->InitializeLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            interstitial_ad_->InitializeLastResult().Error() ==
                firebase::gma::kAdErrorNone) {
          interstitial_ad_->LoadAd(createRequest());
        }
      }
      break;
    case 5:
      if (kTestInterstitialAd) {
        // Show the interstitial ad.
        if (interstitial_ad_->LoadAdLastResult().Status() ==
                firebase::kFutureStatusComplete &&
            interstitial_ad_->LoadAdLastResult().Error() ==
                firebase::gma::kAdErrorNone &&
            interstitial_ad_->ShowLastResult().Status() !=
                firebase::kFutureStatusComplete) {
          interstitial_ad_->Show();
        }
      }
      break;
    default:
      break;
  }
}

// The vertex shader code string.
static const GLchar* kVertexShaderCodeString =
    "attribute vec2 position;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}";

// The fragment shader code string.
static const GLchar* kFragmentShaderCodeString =
    "precision mediump float;\n"
    "uniform vec4 myColor; \n"
    "void main() { \n"
    "    gl_FragColor = myColor; \n"
    "}";

// Creates the OpenGL surface.
void GameEngine::onSurfaceCreated() {
  vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vertex_shader_, 1, &kVertexShaderCodeString, NULL);
  glCompileShader(vertex_shader_);

  GLint status;
  glGetShaderiv(vertex_shader_, GL_COMPILE_STATUS, &status);

  char buffer[512];
  glGetShaderInfoLog(vertex_shader_, 512, NULL, buffer);

  glShaderSource(fragment_shader_, 1, &kFragmentShaderCodeString, NULL);
  glCompileShader(fragment_shader_);

  glGetShaderiv(fragment_shader_, GL_COMPILE_STATUS, &status);

  glGetShaderInfoLog(fragment_shader_, 512, NULL, buffer);

  shader_program_ = glCreateProgram();
  glAttachShader(shader_program_, vertex_shader_);
  glAttachShader(shader_program_, fragment_shader_);

  glLinkProgram(shader_program_);
  glUseProgram(shader_program_);
}

// Updates the OpenGL surface.
void GameEngine::onSurfaceChanged(int width, int height) {
  width_ = width;
  height_ = height;

  GLfloat heightIncrement = 0.25f;
  GLfloat currentHeight = 0.93f;

  for (int i = 0; i < kNumberOfButtons; i++) {
    int base = i * 8;
    vertices_[base] = -0.9f;
    vertices_[base + 1] = currentHeight;
    vertices_[base + 2] = 0.9f;
    vertices_[base + 3] = currentHeight;
    vertices_[base + 4] = -0.9f;
    vertices_[base + 5] = currentHeight - heightIncrement;
    vertices_[base + 6] = 0.9f;
    vertices_[base + 7] = currentHeight - heightIncrement;
    currentHeight -= 1.2 * heightIncrement;
  }
}

// Draws the frame for the OpenGL surface.
void GameEngine::onDrawFrame() {
  glClearColor(0.0f, 0.0f, bg_intensity_, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_, GL_STATIC_DRAW);

  GLfloat colorBytes[] = {0.9f, 0.9f, 0.9f, 1.0f};
  GLint colorLocation = glGetUniformLocation(shader_program_, "myColor");
  glUniform4fv(colorLocation, 1, colorBytes);

  GLint posAttrib = glGetAttribLocation(shader_program_, "position");
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(posAttrib);

  for (int i = 0; i < kNumberOfButtons; i++) {
    glDrawArrays(GL_TRIANGLE_STRIP, i * 4, 4);
  }
}
