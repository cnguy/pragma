'use strict';

const autoprefixer = require('autoprefixer');
const path = require('path');
const webpack = require('webpack');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const UglifyJsPlugin = require("uglifyjs-webpack-plugin");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const SWPrecacheWebpackPlugin = require('sw-precache-webpack-plugin');
const CleanWebpackPlugin = require('clean-webpack-plugin');
const ModuleScopePlugin = require('react-dev-utils/ModuleScopePlugin');
const manifestPlugin = require('./manifest');
const paths = require('./paths');
const getClientEnvironment = require('./env');

const publicPath = "./";
const shouldUseRelativeAssetPaths = publicPath === './';
const shouldUseSourceMap = false;
const env = getClientEnvironment();

if (env.stringified['process.env'].NODE_ENV !== '"production"') {
  throw new Error('Production builds must have NODE_ENV=production.');
}

const cssFilename = 'css/[name].[contenthash:8].css';

const miniCssExtractluginOptions = shouldUseRelativeAssetPaths
  ? // Making sure that the publicPath goes back to to build folder.
    { publicPath: Array(cssFilename.split('/').length).join('../') }
  : {};

const optimization = {
  splitChunks: {
    chunks: 'async',
    cacheGroups: {
      vendors: {
        chunks: 'all',
        test: /[\\/]node_modules[\\/]/,
        priority: -10
      },
      default: {
        minChunks: 2,
        priority: -20,
        reuseExistingChunk: true
      },
      cmModes: {
        test: /[\\/]node_modules[\\/]codemirror/,
        minSize: 0,
        name: (mod, _chunks) => {
          const builtInMode = /node_modules\/codemirror\/mode\/(\w+)\//.exec(
            mod.resource
          )
          if (builtInMode) {
            return `cm-mode/${builtInMode[1]}`
          }
          const external = /node_modules\/codemirror-mode-(\w+)\//.exec(
            mod.resource
          )
          if (external) {
            return `cm-ext/${external[1]}`
          }
          return 'cm-common';
        },
      },
    }
  },
  minimize: true,
  namedModules: false,
  minimizer: [
    new UglifyJsPlugin({
      cache: true,
      parallel: true,
      sourceMap: shouldUseSourceMap
    }),
    new OptimizeCSSAssetsPlugin({})
  ]
};

module.exports = {
  mode: "production",
  optimization,
  bail: true,
  devtool: shouldUseSourceMap ? 'source-map' : false,
  entry: [require.resolve('./polyfills'), paths.appIndexJs],
  output: {
    path: paths.appBuild,
    filename: 'js/[name].[chunkhash:8].js',
    chunkFilename: 'js/[name].[chunkhash:8].chunk.js',
    publicPath: publicPath,
    devtoolModuleFilenameTemplate: info =>
      path
        .relative(paths.appSrc, info.absoluteResourcePath)
        .replace(/\\/g, '/'),
  },
  resolve: {
    modules: ['node_modules', paths.appNodeModules].concat(
      process.env.NODE_PATH.split(path.delimiter).filter(Boolean)
    ),
    extensions: ['.re', '.ml', '.js', '.json', '.jsx'],
    plugins: [
      new ModuleScopePlugin(paths.appSrc, [paths.appPackageJson]),
    ],
  },
  module: {
    strictExportPresence: true,
    rules: [
      {
        oneOf: [
          {
            test: [/\.bmp$/, /\.gif$/, /\.jpe?g$/, /\.png$/, /\.svg$/],
            loader: require.resolve('url-loader'),
            options: {
              limit: 10000,
              name: 'assets/[name].[hash:8].[ext]',
            },
          },
          {
            test: /\.(js|jsx|mjs)$/,
            include: paths.appSrc,
            loader: require.resolve('babel-loader'),
            options: {
              
              compact: true,
            },
          },
          {
            test: /\.css$/,
            include: paths.appNodeModules,
            use: [
              {
                loader: MiniCssExtractPlugin.loader,
                options: miniCssExtractluginOptions
              },
              {
                loader: require.resolve('css-loader'),
                options: {
                  importLoaders: 1,
                  minimize: true,
                  sourceMap: shouldUseSourceMap,
                }
              }
            ]
          },
          {
            test: /\.s?css$/,
            use: [
              {
                loader: MiniCssExtractPlugin.loader,
                options: miniCssExtractluginOptions
              },
              {
                loader: require.resolve('css-loader'),
                options: {
                  importLoaders: 1,
                  minimize: true,
                  modules: true,
                  sourceMap: shouldUseSourceMap,
                },
              },
              {
                loader: require.resolve('postcss-loader'),
                options: {
                  ident: 'postcss',
                  plugins: () => [
                    require('postcss-flexbugs-fixes'),
                    autoprefixer({
                      browsers: [
                        '>1%',
                        'last 4 versions',
                        'Firefox ESR',
                        'not ie < 9', // React doesn't support IE8 anyway
                      ],
                      flexbox: 'no-2009',
                    }),
                  ],
                },
              },
              require.resolve('sass-loader')
            ],
          },
          {
            loader: require.resolve('file-loader'),
            exclude: [/\.js$/, /\.mjs$/, /\.html$/, /\.json$/],
            options: {
              name: 'assets/[name].[hash:8].[ext]',
            },
          },
        ],
      },
    ],
  },
  plugins: [
    new CleanWebpackPlugin(paths.appBuild, { root: process.cwd() }),
    new HtmlWebpackPlugin({
      inject: true,
      template: paths.appHtml,
      minify: {
        removeComments: true,
        collapseWhitespace: true,
        removeRedundantAttributes: true,
        useShortDoctype: true,
        removeEmptyAttributes: true,
        removeStyleLinkTypeAttributes: true,
        keepClosingSlash: true,
        minifyJS: true,
        minifyCSS: true,
        minifyURLs: true,
      },
    }),
    manifestPlugin,
    new webpack.DefinePlugin(env.stringified),
    new MiniCssExtractPlugin({
      filename: cssFilename,
    }),
    new SWPrecacheWebpackPlugin({
      // By default, a cache-busting query parameter is appended to requests
      // used to populate the caches, to ensure the responses are fresh.
      // If a URL is already hashed by Webpack, then there is no concern
      // about it being stale, and the cache-busting can be skipped.
      dontCacheBustUrlsMatching: /\.\w{8}\./,
      filename: 'service-worker.js',
      logger(message) {
        if (message.indexOf('Total precache size is') === 0) {
          return;
        }
        if (message.indexOf('Skipping static resource') === 0) {
          return;
        }
        console.log(message);
      },
      minify: true,
      navigateFallback: "/",
      navigateFallbackWhitelist: [/^(?!\/__).*/],
      staticFileGlobsIgnorePatterns: [/\.map$/],
    }),
  ],
  node: {
    dgram: 'empty',
    fs: 'empty',
    net: 'empty',
    tls: 'empty',
    child_process: 'empty',
  },
};
